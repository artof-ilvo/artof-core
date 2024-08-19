#include <Utils/Geometry/Line.h>
#include <Utils/Geometry/Angle.h>


using namespace Ilvo::Utils::Geometry;
using namespace std;

Line::Line() : pointStart(Point()), pointStop(Point()), empty(true) {}

Line::Line(Point p1, Point p2) : pointStart(p1), pointStop(p2), empty(false) {}

Line& Line::operator= (const Line& from)
{
    empty = false;
    pointStart = from.p1();
    pointStop = from.p2();
    return *this;
}


const Point& Line::p1() const
{
    return pointStart;
}


const Point& Line::p2() const
{
    return pointStop;
}


void Line::p1(Point p1)
{
    empty = false;
    pointStart = p1;
}


void Line::p2(Point p2)
{
    empty = false;
    pointStop = p2;
}


bool Line::isEmpty() const
{
    return empty;
}


/**
 * @brief Line parameters a * x + b * y + c = 0
 *  (y - yA) = ((yB - yA)/(xB - xA)) * (x - xA)
 *  (xB - xA)*y - yA*(xB - xA) = x*(yB - yA) - xA*(yB - yA)
 *  x * (yA - yB) + y * (xB - xA) +  (xA*yB - xB*yA) = 0
 *  a * x + b * y + c = 0
 * @param a 
 * @param b 
 * @param c 
 */
void Line::params(double &a, double &b, double &c)
{
    a = pointStart.y() - pointStop.y(); // a = (yA - yB) 
    b = pointStop.x() - pointStart.x(); // b = (xB - xA)
    c = pointStart.x() * pointStop.y() - pointStop.x() * pointStart.y(); // c = (xA*yB - xB*yA)
}

/**
 * @brief Line parameters y = m * x + q
 *  m = (yB - yA) / (xB - xA) = - (a / b)
 *  q = yA - m * xA = - (c / b)
 * @param m 
 * @param q 
 */
void Line::params(double &m, double &q)
{
    double a,b,c;
    params(a, b, c);
    m = - (a / b);
    q = - (c / b);
}


/**
 * @brief Get the angle alpha of the line to the x axis in a cartesian coordinate system (in degrees)
 * 
 * @return double 
 */
double Line::alpha()
{
    return RadToDeg(atan2( (pointStop.y() - pointStart.y()), (pointStop.x() - pointStart.x()) ));
}

/**
 * @brief Extend the line
 * 
 * @param distance Distance to extend the line
 * @param side Side of the line to extend (BEGIN, END, BOTH)
 */
void Line::extend(double distance, Side side) 
{
    double alphaRadians = DegToRad(alpha());
    if (side == Side::BEGIN) {
        pointStart.x(pointStart.x() - distance * cos(alphaRadians));
        pointStart.y(pointStart.y() - distance * sin(alphaRadians));
    } else if (side == Side::END) {
        pointStop.x(pointStop.x() + distance * cos(alphaRadians));
        pointStop.y(pointStop.y() + distance * sin(alphaRadians));
    } else {
        extend(distance, Side::BEGIN);
        extend(distance, Side::END);
    }
}

/**
 * @brief Point on the line, from the beginning or the end
 * 
 * @param distance Distance from the beginning or the end
 * @param side Side of the line (BEGIN, END)
 * @return Point 
 */
Point Line::pointFrom(double distance, Side side) 
{
    double alphaRadians = DegToRad(alpha());
    if (side == Side::BEGIN) {
        return Point(pointStart.x() - distance * cos(alphaRadians), pointStart.y() - distance * sin(alphaRadians));
    } else {
        return Point(pointStop.x() + distance * cos(alphaRadians), pointStop.y() + distance * sin(alphaRadians));
    } 
}

/**
 * @brief Create an orthogonal line to this line at the point pointStart.
 * 
 * @param pointStart the point on this line from which the orthogonal line starts
 * @param length the length of the orthogonal line
 * @param left left or right from this line
 * @return Line (orthogonal line)
 */
Line Line::orthogonal(Point pointStart, double length, bool left)
{
    double a,b,c;
    params(a, b, c);
    double m = - (a / b);
    double q = - (c / b);
    
    Point p;
    if (m == 0) {
        p.x(pointStart.x());
        p.y(pointStart.y() + (left? 1 : -1) * length);
    } else {
        double m_new = -1 / m;
        double q_new = pointStart.y() - m_new * pointStart.x();

        double x1 = pointStart.x() - (sqrt(pow(length, 2) / (1 + pow(m_new, 2))));
        double y1 = m_new * x1 + q_new;
        Point p1(x1, y1);

        double x2 = pointStart.x() + (sqrt(pow(length, 2) / (1 + pow(m_new, 2))));
        double y2 = m_new * x2 + q_new; 
        Point p2(x2, y2);

        if (this->left(p1) == left) {
            p = p1;
        } else {
            p = p2;
        } 
    }

    return Line(pointStart, p);
}

/**
 * @brief distance to point
 * 
 * @param point Point to which the smallest distance to this line is calculated
 * @return double 
 */
double Line::distance(Point point)
{
    double a, b, c;
    params(a, b, c);
    return std::abs(a * point.x() + b * point.y() + c) / std::sqrt(a * a + b * b);
}

/**
 * @brief Returns point C lies left from line AB
 * 
 * @param A First point of line AB
 * @param B Second point of line AB
 * @param C Point referenced to AB
 * @return true if C lies left from line AB else false
 */
bool Line::left(Point c) 
{
    return ((pointStop.x() - pointStart.x()) * (c.y() - pointStart.y()) - (pointStop.y() - pointStart.y()) * (c.x() - pointStart.x() )) > 0;
}

/**
 * @brief Returns the angle between line AB and CD.
 * 
 * @param A First point of line AB
 * @param B Second point of line AB
 * @param C First point of line CD
 * @param D Second point of line CD
 * @return double: Angle in degrees 
 */
double Line::corner(Line line)
{
    return calcSmallestAngleAbsolute(alpha(), constrainAngle(line.alpha() + 180));
}

/**
 * @brief Returns the center point of the line
 * 
 * @return Point : center point
 */
Point Line::center()
{
    return Point((pointStart.x() + pointStop.x()) / 2, (pointStart.y() + pointStop.y()) / 2);
}

/**
 * @brief Searches the point where line AB and CD intersects.
 * 
 * @param A First point of line AB
 * @param B Second point of line AB
 * @param C First point of line CD
 * @param D Second point of line CD
 * @return Point: point of intersection
 */
Point Line::intersection(Line line)
{
    Point A = pointStart;
    Point B = pointStop;
    Point C = line.pointStart;
    Point D = line.pointStop;

    // Line AB represented as a1x + b1y = c1
    double a1 = B.y() - A.y();
    double b1 = A.x() - B.x();
    double c1 = a1*(A.x()) + b1*(A.y());

    // Line CD represented as a2x + b2y = c2
    double a2 = D.y() - C.y();
    double b2 = C.x() - D.x();
    double c2 = a2*(C.x())+ b2*(C.y());

    double determinant = a1*b2 - a2*b1;

    if (determinant == 0)
    {
        // The lines are parallel. This is simplified
        // by returning a pair of FLT_MAX
        return Point(FLT_MAX, FLT_MAX);
    }
    else
    {
        double x = (b2*c1 - b1*c2)/determinant;
        double y = (a1*c2 - a2*c1)/determinant;
        return Point(x, y);
    }
}

vector<PointPtr> Line::interpolate(double interpolationDistance, bool curvy) const
{
    vector<PointPtr> interpolation;

    double d = pointStart.distance(pointStop);
    int num = int(d/interpolationDistance);
    double dx = (pointStop.x() - pointStart.x()) / num;
    double dy = (pointStop.y() - pointStart.y()) / num;

    for (int j = 0; j < num; j++) {
        double x = pointStart.x() + j * dx;
        double y = pointStart.y() + j * dy;
        if (curvy) {
            interpolation.push_back(make_shared<CurvyPoint>(x, y));
        } else {
            interpolation.push_back(make_shared<Point>(x,y));
        }
    }

    return interpolation;
}

double Line::length() const
{
    return pointStart.distance(pointStop);
}