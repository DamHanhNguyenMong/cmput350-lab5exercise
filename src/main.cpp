#include <iostream>
#include <optional>
#include <vector>
#include <cmath>
#include <limits>


#include <SFML/Graphics.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;

using Point2D = sf::Vector2f;

// DrawLine 
void drawLine(sf::RenderWindow& window,
              Point2D from,
              Point2D to,
              float width,
              sf::Color color) {

    sf::ConvexShape drawableLine(4);

    // Vector perpendicular to the line
    Point2D end = Point2D(from.y - to.y, to.x - from.x);

    // Normalize and scale to half the line width
    end = end.normalized() * (width / 2.0f);

    // Four corners of the thick line
    Point2D p1 = from - end;
    Point2D p2 = from + end;
    Point2D p3 = to + end;
    Point2D p4 = to - end;

    drawableLine.setPoint(0, p1);
    drawableLine.setPoint(1, p2);
    drawableLine.setPoint(2, p3);
    drawableLine.setPoint(3, p4);

    drawableLine.setFillColor(color);

    window.draw(drawableLine);
}

void drawCircle(sf::RenderWindow& window,
                Point2D p,
                float radius,
                sf::Color color) {

    sf::CircleShape drawableCircle(radius);

    // Center the circle on p
    drawableCircle.setOrigin({radius, radius});

    drawableCircle.setPosition(p);
    drawableCircle.setFillColor(color);

    window.draw(drawableCircle);
}



// TODO: (Part 1) Define a function that samples a cubic Bezier curve at t in [0, 1].
Point2D getPoint(const std::vector<sf::Vector2f>& pts, float t) { 
    // Define u as 1-t for convenience
    float u = 1.0f - t;
    float b0 = u*u*u; //(1-t)^3
    float b1 = 3.0f*u*u*t; //3(1-t)^2t
    float b2 = 3.0f*u*t*t; //3(1-t)t^2
    float b3 = t*t*t; //t^3

    float x = b0 * pts[0].x + b1*pts[1].x + b2*pts[2].x + b3*pts[3].x;
    float y = b0 * pts[0].y + b1*pts[1].y + b2*pts[2].y + b3*pts[3].y;

    return Point2D{x,y}; }

// TODO: (Part 2) Define a function that returns the curve's slope at t in [0, 1].
Point2D getSlope(const std::vector<sf::Vector2f>& pts, float t) { 
    // Define u as 1-t for convenience
    float u = 1.0f - t;
    float b0 = 3.0f * u * u; // derivative of u^3 -> 3u^2
    float b1 = 6.0f * u * t; // derivative of -3(1-t)^2t and 3(1-t)t^2 -> 6*u*t
    float b2 = 3.0f * t * t; // derivative of t^3 -> 3t^2
    
    // b(t) = u^3P0+3u^2tP1+3ut^2P2+t^3P3
    // b'(t) = 3u^2(P1-P0)+6ut(P2-P1) +3t^2(P3-P2)
    float x = b0 * (pts[1].x - pts[0].x) + b1 * (pts[2].x - pts[1].x) + b2 * (pts[3].x - pts[2].x);
    float y = b0 * (pts[1].y - pts[0].y) + b1 * (pts[2].y - pts[1].y) + b2 * (pts[3].y - pts[2].y);
    return Point2D{x,y}; }

// TODO: (Part 1) Store four control points for the curve.
std::vector<sf::Vector2f> controlPoints = {
    {100.0f, 600.0f},
    {250.0f, 100.0f},
    {550.0f, 100.0f},
    {700.0f, 600.0f}
};

// TODO: (Part 2) Track animation time for the square moving along the curve.
float animationTime = 0.0f;

// TODO: (Part 3) Track the index of the control point being dragged.
int draggedPoint = -1; // no point is currently being dragged

// Helper function: Maintain the curve as smooth. As we connect other 4 point of a bezier curve. 1 point will be shared
// This function makes sure the direction between the prev, shared, next points are the same
void maintainSmoothCurve(int pointIndex) {
    // Keep the next point in the same direction from the shared point
    // as the previous point, while keeping the same distance.

    if (pointIndex + 2 >= static_cast<int>(controlPoints.size())) {
        return; 
    }
    Point2D previous = controlPoints[pointIndex];
    Point2D shared = controlPoints[pointIndex + 1];
    Point2D next = controlPoints[pointIndex + 2];
    Point2D direction = shared - previous; // calculate the direction between prev and shared point

    //Distance between next shared point and the shared point
    float distance = std::sqrt((next.x - shared.x) * (next.x - shared.x) + (next.y - shared.y) * (next.y - shared.y));

    // Distance between prev and shared
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    // Avoid dividing by zero
    if (length > 0.0f) {
        direction.x /= length;
        direction.y /= length; // unit direction vec
        controlPoints[pointIndex + 2] = shared + direction * distance; //maintain the direction and the length
    }
}


void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            shouldQuit = true;
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            // TODO: (Part 3) On left-click, select the closest control point
            // using mouse->position and start dragging it.

            //If left mouse is pressed
            if (mouse->button == sf::Mouse::Button::Left) {
                // get the position the mouse was pressed and convert to vector
                Point2D mousePosition( static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y));
                // Find the closest control point
                float closestDistance = std::numeric_limits<float>::max(); // get a large num
                draggedPoint = -1; //Reset the selected point

                //Loop through all the control points (4)
                for (int i = 0; i < static_cast<int>(controlPoints.size()); ++i){
                    // difference between the control point's x-coordinate and the mouse's x-coordinate
                    float dx = controlPoints[i].x - mousePosition.x;
                    // And for y-coordinate
                    float dy = controlPoints[i].y - mousePosition.y;

                    //Distance
                    float distanceSquared = dx * dx + dy * dy; //sqare distance
                    // Check if this is the closest point
                    if (distanceSquared < closestDistance) {
                        closestDistance = distanceSquared;
                        draggedPoint = i;
                    }

                }

            }
        } else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>()) {
            // TODO: (Part 3) On left-button release, stop dragging.
            if (mouse->button == sf::Mouse::Button::Left) {
                draggedPoint = -1; // no longer dragging
            }

        } else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>()) {
            // TODO: (Part 3) Move the selected control point to mouse->position.

            // If we are dragging
            if (draggedPoint != -1) {
                // move the control point position to the current mouse position
                controlPoints[draggedPoint] = Point2D(static_cast<float>(mouse->position.x),static_cast<float>(mouse->position.y));
            
            // TODO: (Part 4) Maintain matching slopes at shared endpoints.
            // When moving point 3, move point 5 without changing its distance
            // from point 4 (point numbers here start at 1).
                if (draggedPoint >= 2 && draggedPoint % 3 == 2) {
                    maintainSmoothCurve(draggedPoint);
                }
            }
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            // TODO: (Part 4) '+' adds three control points; '-' removes three,
            // keeping at least four points.

            // Add three control points
            if (key->code == sf::Keyboard::Key::Add || key->code == sf::Keyboard::Key::Equal) {
                // Get the last control point on the stack
                Point2D last = controlPoints.back();
                // going leftward keeps the points on screen
                Point2D prevHandle = controlPoints[controlPoints.size() - 2];

                controlPoints.push_back(last + (last - prevHandle));      // mirrored, so it starts smooth
                controlPoints.push_back(last + Point2D(-100.0f, -150.0f));
                controlPoints.push_back(last + Point2D(-200.0f, 0.0f));
                        }
            // Remove three control points
            else if (key->code == sf::Keyboard::Key::Subtract || key->code == sf::Keyboard::Key::Hyphen) {
                // Only remove if there are currently = or more than 7 points
                if (controlPoints.size() >= 7) {
                    controlPoints.pop_back();
                    controlPoints.pop_back();
                    controlPoints.pop_back();
                }
                // If draggedPoint is one of the removed indices, the next mouse move indexes past the end of the vector
                if (draggedPoint >= static_cast<int>(controlPoints.size())) draggedPoint = -1;
            }
        }
    }
}

void render(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);

    // ====== ====== ======
    // TODO: (Part 2) Draw a small square moving repeatedly along the curve.
    // Use GetSlope to orient it to the curve at each time step.
    // ====== ====== ======
    // Divide segment, each of 4 
    int numSegments = (controlPoints.size() - 1) / 3;
    animationTime += 0.01f;
    // Repeat if over
    if (animationTime >= numSegments) {
        animationTime = 0.0f;
    }
    // Casting to int chops off the decimals
    int seg = static_cast<int>(animationTime);
    // how far through the segment the square is
    float localT = animationTime - seg;
    // grabs the 4 control points for that segment
    std::vector<Point2D> pts = {controlPoints[seg*3], controlPoints[seg*3+1],
                                controlPoints[seg*3+2], controlPoints[seg*3+3]};
    Point2D position = getPoint(pts, localT);
    Point2D slope = getSlope(pts, localT);

    //Calculate the angle: angle = argtan(slope)*180/pi
    float angle = std::atan2(slope.y, slope.x) * 180.0f / 3.14159265f;

    sf::RectangleShape square({20.0f, 20.0f});
    square.setFillColor(sf::Color::Yellow);

    // Put the origin in the center so rotation happens around the middle
    square.setOrigin({10.0f, 10.0f});

    // Move it to the curve
    square.setPosition(position);

    // Rotate it to follow the curve
    square.setRotation(sf::degrees(angle));

    window.draw(square);

    // ====== ====== ======
    // TODO: (Part 3) Draw control handles from point 1 to 2 and point 3 to 4.
    // TODO: (Part 4) Draw all connected cubic Bezier segments and their handles.
    // ====== ====== ======

    // call getPoint() separately for each group of four points
    // every point at position 3 is shared so thats why we +3
    for (int segment = 0; segment + 3 < static_cast<int>(controlPoints.size()); segment += 3) {
        for (int i = 0; i < 100; ++i)
        {
            float t1 = static_cast<float>(i) / 100.0f;
            float t2 = static_cast<float>(i + 1) / 100.0f;

            std::vector<sf::Vector2f> points = {
                controlPoints[segment],
                controlPoints[segment + 1],
                controlPoints[segment + 2],
                controlPoints[segment + 3]
            };

            Point2D p1 = getPoint(points, t1);
            Point2D p2 = getPoint(points, t2);

            drawLine(window, p1, p2, 2.0f, sf::Color::White);
        }
    }
    // Draw handles between control points
    for (int i = 0; i + 3 < static_cast<int>(controlPoints.size()); i += 3) {
        drawLine(window, controlPoints[i], controlPoints[i + 1], 1.0f, sf::Color::Green);
        drawLine(window, controlPoints[i + 2], controlPoints[i + 3], 1.0f, sf::Color::Green);
    }

    // Draw control points on top
    for (const auto& point : controlPoints) {
        drawCircle(window, point, 8.0f, sf::Color::Red);
    }


    // ====== ====== ======
    // TODO: (Bonus) Support multiple curves, a Galaga screen overlay at a 1:2 ratio, and exporting
    // curve points as C++ code for Project 1b.
    // ====== ====== ======

    window.display();
}

int main() {
    sf::RenderWindow window;

    try {
        // Initialize window
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Bezier Curve Editor");
        window.setFramerateLimit(FPS_LIMIT);
        // Prevent key repeats.
        window.setKeyRepeatEnabled(false);

        bool shouldQuit = false;
        // Main game loop
        while (window.isOpen()) {
            handleInput(window, shouldQuit);
            if (shouldQuit) {
                break;
            }
            render(window);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
