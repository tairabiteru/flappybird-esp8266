/**
 * Module defining geometric helper functions and structs.
 * 
 * The functions and structs in here are used pretty much
 * exclusively for computing collisions, which is done with
 * imaginary rectangles around the bird and pipes called
 * "hitboxes". 
 */


/**
* Simple struct defining a point.
*
* @param x The X coordinate.
* @param y The Y coordinate.
*/
struct Point {
  int x, y;
};


/**
* Simple struct defining a rectangle.
*
* @param p1 A Point struct corresponding to the rectangle's upper left corner.
* @param w The width of the rectangle.
* @param h The height of the rectangle.
*/
struct Rect {
  Point p1;
  int w, h;
};


/**
* Function to determine if two rectangles overlap.
*
* How this works exactly is kind of hard to visualize without
* pictures, but regardless. Essentially, we're creating a series
* of points and checking a set of conditions on them to see
* if they do NOT overlap. Since we're looking for the anti-condition
* here, we invert the output, and this gets us what we want.
*
* @param rect1 The first rectangle.
* @param rect2 The second rectangle.
* @returns A boolean which is true if they overlap, and false otherwise.
*/
bool rect_overlap(Rect rect1, Rect rect2) {
  Point l1 = {rect1.p1.x, rect1.p1.y};
  Point r1 = {rect1.p1.x+rect1.w, rect1.p1.y+rect1.h};
  Point l2 = {rect2.p1.x, rect2.p1.y};
  Point r2 = {rect2.p1.x+rect2.w, rect2.p1.y+rect2.h};

  bool cond1 = l1.x > r2.x;
  bool cond2 = r1.x < l2.x;
  bool cond3 = l1.y > r2.y;
  bool cond4 = r1.y < l2.y;
  return !(cond1 || cond2 || cond3 || cond4);
}