#include <Utils/Nmea/Nmea.h>
#include <Utils/Nmea/NmeaMessage.h>
#include <algorithm>    // find
#include <vector>       // vector
#include <sstream>
#include <Utils/Geometry/Transform.h>
#include <Utils/String/String.h>
#include <string>
#include <iostream>


using namespace Ilvo::Utils::Nmea;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::String;
using namespace std;


NmeaLine::NmeaLine(vector<char>& nmeaLine) : 
    nmeaLineStr(string(nmeaLine.begin(), nmeaLine.end())) 
{
    // perform checksum
    if (nmeaChecksum()) {
        parse();
    } 
}

NmeaLine::NmeaLine(NmeaLine& nmeaLine) : 
    nmeaLineStr(nmeaLine.str()), 
    type(nmeaLine.getType())
{
    fieldValues.insert(nmeaLine.getFieldValues().begin(), nmeaLine.getFieldValues().end());
}

void NmeaLine::parse() {
    auto commaIndices = String::getIndexList(nmeaLineStr, ',');

    vector<NmeaField*> fields;
    if (nmeaLineStr.find("GGA") != string::npos) {
        type = NmeaMessageType::GGA;
        fields = GGA_MESSAGE_FORMAT;
    } else if (nmeaLineStr.find("HDT") != string::npos) {
        type = NmeaMessageType::HDT;
        fields = HDT_MESSAGE_FORMAT;
    } else if (nmeaLineStr.find("VTG") != string::npos) {
        type = NmeaMessageType::VTG;
        fields = VTG_MESSAGE_FORMAT;
    } else if (nmeaLineStr.find("HRP") != string::npos) {
        type = NmeaMessageType::HRP;
        fields = HRP_MESSAGE_FORMAT;
    } else {
        nmeaLineStr = "";
        return;
    }     

    
    for (NmeaField* field: fields) {
        int startIdx = 1;
        if (field->idx > 0) {
            startIdx = commaIndices[field->idx-1] + 1;
        }
        int endIdx = commaIndices[field->idx];
        string subStr = string(nmeaLineStr.begin() + startIdx, nmeaLineStr.begin() + endIdx);
        vector<char> subVec = vector<char>(subStr.begin(), subStr.end());

        NmeaFieldValuePtr field_ptr;
        switch (field->type)
        {
        case NmeaFieldType::INT:
        {
            try {
                field_ptr = make_shared<NmeaFieldValue>(field, stoi(subStr));
            } catch(const invalid_argument& e) {   
                field_ptr = make_shared<NmeaFieldValue>(field, (int) 0);
            } 
            break;
        }
        case NmeaFieldType::LONG:
        {
            try {
                field_ptr = make_shared<NmeaFieldValue>(field, stol(subStr));
            } catch(const invalid_argument& e) {   
                field_ptr = make_shared<NmeaFieldValue>(field, (long) 0.0);
            } 
            break;
        }
        case NmeaFieldType::DOUBLE:
        {
            try {
                // special conversions
                double value = 0.0;
                if (field->name == "lat") {
                    if (subVec.size() > 0) value = stod(string(subVec.begin(), subVec.begin()+2)) + stod(string(subVec.begin()+2, subVec.end()))/60;
                } else if (field->name == "lon") {
                    if (subVec.size() > 0) value = stod(string(subVec.begin(), subVec.begin()+3)) + stod(string(subVec.begin()+3, subVec.end()))/60;
                } else {
                    value = stod(subStr);
                }
                field_ptr = make_shared<NmeaFieldValue>(field, value);
            } catch(const invalid_argument& e) {   
                field_ptr = make_shared<NmeaFieldValue>(field, (double) 0.0);
            } 
            break;
        }
        case NmeaFieldType::FLOAT:
        {
            try {
                field_ptr = make_shared<NmeaFieldValue>(field, stof(subStr));
            } catch(const invalid_argument& e) {   
                field_ptr = make_shared<NmeaFieldValue>(field, (float) 0.0);
            } 
            break;
        }
        case NmeaFieldType::STRING: 
        {
            try {
                field_ptr = make_shared<NmeaFieldValue>(field, subStr);
            } catch(const invalid_argument& e) {   
                field_ptr = make_shared<NmeaFieldValue>(field, "");
            } 
            break;
        }
        default:
            nmeaLineStr = "";
            break;
        }
        fieldValues.insert({field->name, field_ptr});
    }
}

bool NmeaLine::nmeaChecksum()
{
    auto idxastrix_ptr = find(nmeaLineStr.begin(), nmeaLineStr.end(), '*');
    unsigned int checksum;
    stringstream ss;
    ss << hex << string(idxastrix_ptr+1, nmeaLineStr.end());
    ss >> checksum;

    int sum = 0;
    for (char x: vector<char>(nmeaLineStr.begin()+1, idxastrix_ptr)) {
        sum = sum ^ (int) x;
    }

    return ((unsigned int) sum == checksum);
}

map<string, NmeaFieldValuePtr>& NmeaLine::getFieldValues() {
    return fieldValues;
}

string& NmeaLine::str() {
    return nmeaLineStr;
}

NmeaMessageType NmeaLine::getType() {
    return type;
}

bool NmeaLine::ok() {
    return nmeaLineStr.size() > 0;
}

ostream& operator<<(ostream& out, NmeaLine& line)
{
    out << line.str() << endl;
    out << "KEY\tTYPE\tELEMENT\n"; 
    for (auto itr = line.getFieldValues().begin(); itr != line.getFieldValues().end(); ++itr) { 
        switch ((itr->second)->type)
        {
        case NmeaFieldType::INT:
            out << itr->first << '\t' << "int" << '\t' << (itr->second)->i << '\n'; 
            break;
        case NmeaFieldType::FLOAT:
            out << itr->first << '\t' << "float" << '\t' << (itr->second)->f << '\n'; 
            break;
        case NmeaFieldType::DOUBLE:
            out << itr->first << '\t' << "double" << '\t' << (itr->second)->d << '\n'; 
            break;
        case NmeaFieldType::LONG:
            out << itr->first << '\t' << "long" << '\t' << (itr->second)->l << '\n'; 
            break;
        case NmeaFieldType::STRING:
            out << itr->first << '\t' << "string" << '\t' << (itr->second)->s << '\n'; 
            break;        
        default:
            break;
        }
        
        
    } 
    return out; 
}
