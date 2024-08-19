#include <Utils/Redis/RedisStream.h>
#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Utils::Redis;
using namespace nlohmann;
using namespace std;
using namespace rediscpp;
using namespace Ilvo::Utils::Logging;

RedisStream::RedisStream(json j) : RedisStream(j["ip"], j["port"]) {}

RedisStream::RedisStream(string ip, int port) : ip(ip), port(port) {
    stream = make_stream(ip, to_string(port));
}

RedisStream::~RedisStream() {
    for (auto& it : subscriberThreads) {
        it.second.second->store(true);
        it.second.first->join();
    }
}

bool RedisStream::isRedisValueNil(string key) {
    string valueStr = getRedisValue(key);
    return valueStr.empty();
}

bool RedisStream::setRedisValues(std::vector<std::string> values) 
{
    if (values.size() > 0) {
        auto response = execute(*(stream), "MSET", values);
        return response.as_string().compare("OK") != 0;
    } 
    return true;
}

deserialization::array::items_type RedisStream::getRedisValues(std::vector<std::string> values)
{
    auto response = execute(*(stream), "MGET", values);
    auto arr = std::get<deserialization::array>(response.get()).get();
    return arr;
}

bool RedisStream::setRedisJsonValue(string name, const nlohmann::json& j)
{
    string value = j.empty() ? "{}" : j.dump();
    auto response = execute(*(stream), "JSON.SET", name, "$", value);
    return response.as_string().compare("OK") != 0;
}

json RedisStream::getRedisJsonValue(string name)
{
    auto response = execute(*(stream), "JSON.GET", name);
    try {
        string value {response.as<string>()};
        std::string s = (std::string) response.as_string();
        // LoggerStream::getInstance() << DEBUG << "value: " << s << std::endl;
        return json::parse(value);
    } catch(std::logic_error& e) {
        LoggerStream::getInstance() << ERROR << "Redis Get Json Value logic error for \"" << name << "\", " << e.what();
        return json();
    } catch(json::exception& e) {
        LoggerStream::getInstance() << ERROR << "Redis Get Json Value parse error for \"" << name << "\", " << e.what();
        return json();
    } catch(std::exception& e) {
        LoggerStream::getInstance() << ERROR << "Redis Get Json Value exception for \"" << name << "\", " << e.what();
        return json();
    }
}

json RedisStream::getRedisJsonValue(string name, json initIfNotExists)
{
    auto response = execute(*(stream), "JSON.GET", name);
    try {
        string value {response.as<string>()};
        std::string s = (std::string) response.as_string();
        // LoggerStream::getInstance() << DEBUG << "value: " << s << std::endl;
        return json::parse(value);
    } catch(std::logic_error& e) {
        setRedisJsonValue(name, initIfNotExists);
        LoggerStream::getInstance() << ERROR << "Redis Get Json Value logic error for \"" << name << "\", " << e.what();
        return initIfNotExists;
    } catch(json::exception& e) {
        LoggerStream::getInstance() << ERROR << "Redis Get Json Value parse error for \"" << name << "\", " << e.what();
        return initIfNotExists;
    }
}


void RedisStream::subscribeRedisValue(std::string& name,std::function<void(const std::string_view&)>& callback) {
    auto detached = std::make_shared<std::atomic<bool>>(false);
    auto thread = std::make_shared<boost::thread>(
        [this, name, callback]
        {
            auto response = rediscpp::execute(*this->stream, "subscribe", name);
            // An almost endless loop for getting messages from the queues.
            while (!this->subscriberThreads[name].second->load())
            {
                // Reading / waiting for a message.
                rediscpp::value value{*stream};
                // Message extraction.
                std::visit(rediscpp::resp::detail::overloaded{
                        // We're wondered only an array in response.
                        // Otherwise, there is an error.
                        [callback, name] (rediscpp::resp::deserialization::array const &arr)
                        {
                            for (int i = 2; i < arr.size(); i++) {
                                auto const &item = arr.get()[i];
                                using namespace rediscpp::resp::deserialization;
                                std::visit(rediscpp::resp::detail::overloaded{
                                    [callback] (bulk_string const &item)
                                    { callback(item.get());},
                                    [name] (auto const &)
                                    { LoggerStream::getInstance() << ERROR << "Redis SUBSCRIBE " << name << "Unexpected value type."; }
                                }, item);
                            }
                        },
                        // An error in a response.
                        [name] (rediscpp::resp::deserialization::error_message const &err)
                        { LoggerStream::getInstance() << ERROR << "Redis SUBSCRIBE " << name << ": " << err.get(); },
                        // An unexpected response.
                        [name] (auto const &)
                        { LoggerStream::getInstance() << ERROR << "Redis SUBSCRIBE " << name << "Unexpected value type."; }
                    }, value.get());
            }
        }
    );

    subscriberThreads[name] = std::make_pair(std::move(thread), std::move(detached));
}

void RedisStream::detachSubscribeRedisValue(std::string& name) {
    subscriberThreads[name].second->store(true);
}

void RedisStream::unsubscribeRedisValue(std::string& name) {
    subscriberThreads[name].first->join();
    subscriberThreads.erase(name);
}