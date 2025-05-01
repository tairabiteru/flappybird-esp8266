/*
Author: Taira

This program started when I created a little box with a screen,
button, and ESP8266, and wrote a simple program to roll a little
die on the screen; an electronic die roller of sorts.
That got me to thinking, "hmmm...I have a button, a screen, and a
computer......that's everything you need to play Flappy Bird."

And so I decided to write Flappy Bird for the ESP8266.
Why?

"Science isn't about 'WHY', it's about 'WHY NOT'"
- Cave Johnson
*/

/*
I use Adafruit_GFX for display rendering, and Adafruit's SDD1306 driver.
The SDD1306 is the display chip on the screen I'm using.
*/
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
Wire.h is used for I2C communication.
*/
#include <Wire.h>

/*
ESP8266TrueRandom is used for the randomness used by the program.
(Pipe positioning, speed, etc...)
*/
#include <ESP8266TrueRandom.h>

/*
The following files were written by me:

- geometry.hpp contains helper functions and classes defining geometric behavior.
  This is how the system can understand points, lines, and (most critically) rectangles.
  The concept of bounding rectangles is central to the game's collision detection.

- bitmap.hpp contains bitmaps like the bird, title screen logo, and pieces of the pipes.

- button.hpp contains helper classes abstracting button behavior. It allows me to
  (for example) ignore the fact that the button's output is actually inverted because
  they are in INPUT_PULLUP mode. They also define shortcut member functions for
  awaiting clicks, held buttons, etc...

- score.hpp contains two tiny functions which read and write to EEPROM.
  This (as its name implies) is used for the storage of high scores in order
  to persist them past power loss.
*/
#include <geometry.hpp>
#include <bitmap.hpp>
#include <button.hpp>
#include <score.hpp>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C // OLED I2C bus address

#define A_BUTTON 12 // A button GPIO input
#define RST_BUTTON 14 // Reset button GPIO input

#define FRAMERATE 60 // Target 60 FPS, or about 17 ms delay per frame.
// Be careful setting this higher, as it's limited by the bandwidth of I2C, which is not very high.


// Connection on GPIO 5 and 4.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Button a_button(A_BUTTON, InputMode::INVERTED);
Button rst_button(RST_BUTTON, InputMode::INVERTED);

// Game variables

// Says whether or not the game is currently in progress.
bool game_started = false;

// The height of the bird in pixels.
// Starts at 26, approximately the middle of the screen.
int bird_height = 26;

// Array abstracting a pipe on the screen.
// The first two elements are the x and y coordinates on the screen.
// The last value is the distance between the two pipes in pixels.
int pipe[3] = {-15, 0, 0};
/*
"Ok, but why are we setting the first value to -15?" I hear you cry.
Well, it's ultimately because that's how wide pipes are. The program
determines when to make a new pipe by checking the X coordinate of the
pipe. When the pipes reach a position of -14 pixels, it means only the
very rightmost edge of the pipe remains on the left side of the screen.
At -15, the pipe is completely off the screen. This is how the pipe
rendering system determines it's time to make a new pipe, and since we
want the program to start by rendering a pipe, we set this to -15.

Kind of convoluted, I know. All of programming is convoluted, so get
used to it.
*/

// Holds the current score.
int score = -1;

// The advance value determines by how many pixels the pipes
// should move towards you in each frame. The value is later
// determined randomly, adding a degree of difficulty.
int advance = 5;

/**
 * Display title screen.
 * 
 * This just displays the title screen at the beginning
 * when the MCU is first powered up.
 */
void title_screen() {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, 1);
  display.drawRect(1, 1, 126, 62, 1);
  display.drawRect(2, 2, 124, 60, 1);
  display.drawBitmap(20, 6, title_logo, 90, 24, 1);
  display.drawBitmap(54, 40, BIRD, 20, 12, 1);
  display.setTextSize(1);
  display.setCursor(16, 48);
  display.setTextColor(WHITE);
  display.println("Press        A");
  display.display();
}


/**
 * Reset the game.
 * 
 * This function initializes all variables listed above
 * and has the action of resetting the game completely. 
 */
void reset() {
  game_started = false;
  bird_height = 26;
  pipe[0] = -15;
  score = -1;
  display.clearDisplay();
  title_screen();
}


/**
 * Perform game over actions.
 * 
 * This function is responsible for halting the
 * game's progression and displaying the game over
 * screen, as well as displaying (and saving, if 
 * applicable) the high score.
 */
void game_over() {
  // Setting this false has the action of halting the
  // game's progression.
  game_started = false;

  // Draw the window on top of the game to display text
  display.drawRect(32, 8, 96, 48, 1);
  display.drawRect(33, 9, 94, 46, 1);
  display.drawRect(34, 10, 92, 44, 1);
  display.fillRect(35, 11, 90, 42, BLACK);

  // Display game over text
  display.setTextSize(1);
  display.setCursor(40, 16);
  display.setTextColor(WHITE);
  display.println("Game Over!");

  // Retrieve and display score text
  display.setCursor(40, 25);
  display.print("Score: ");
  display.print(score);
  display.setCursor(40, 34);
  display.print("Best: ");
  int high_score = get_high_score();
  display.print(high_score);

  // Write out to display
  display.display();

  // If the player got a high score,
  // that score is saved to EEPROM here.
  if (score > high_score) {
    set_high_score(score);
  }

  // Wait for the reset button to be clicked.
  rst_button.await_click();
  reset();
}


/**
 * Draw the bird on the screen.
 * 
 * This function acts as a shortcut to draw the bird.
 * Since the bird always remains in the same place along
 * the X axis, this function only takes the
 * height of the bird.
 * 
 * @param height The height off of the ground in pixels.
 */
void draw_bird(int height) {
  display.drawBitmap(0, height, BIRD, 20, 12, 1);
}


/**
 * Draw a pipe at the specified X and Y coordinates.
 * 
 * This function works to draw pipes by drawing the "top"
 * of the pipe at the specified X,Y coordinates, then it draws
 * the remainder of the pipe to the top or bottom of the screen.
 * 
 * @param x The X coordinate to draw the pipe at.
 * @param y The Y coordinate to draw the pipe at.
 * @param upside_down Whether or not the pipe is upside down.
 */
void draw_pipe(int x, int y, bool upside_down) {
  // Check to see if the pipe is upside down.
  // If it is, all of the math is basically reversed.
  if (upside_down == false) {
    // Draw the top of the pipe.
    display.drawBitmap(x, y, PIPE_UP, 15, 5, 1);
    // The top of the pipe is 5 pixels high, so move down by
    // 5 pixels.
    y = y + 5;

    // Now loop
    while (y < SCREEN_HEIGHT) {
      // Draw a section of the pipe which is 1 pixel high.
      display.drawBitmap(x, y, PIPE_EXT, 15, 1, 1);
      // Then move down 1
      y = y + 1;
      // Keep doing this until we reach the height of the screen.
      // (Thus, completing the pipe)
    }

  // This is the same thing, but backwards for upside down pipes.
  } else {
    y = y - 5;
    display.drawBitmap(x, y, PIPE_DOWN, 15, 5, 1);
    y = y - 1;
    while (y > 0) {
      display.drawBitmap(x, y, PIPE_EXT, 15, 1, 1);
      y = y - 1;
    }
  }
}


/**
 * Function abstracting the drawing of a pair of pipes.
 * 
 * If you think about it, we don't even need to consider
 * the positioning of both pipes. Since the X coordinate of
 * both pipes will always be the same, we can basically ignore
 * it for one of them, instead specifying an offset between the
 * pipes, or a "gap" if you will.
 * Thus, drawing a pair of pipes is simplified into an X and Y
 * coordinate, and the distance between the top and bottom pipes.
 * 
 * @param x The X coordinate to draw the bottom pipe at.
 * @param y The Y coordinate to draw the bottom pipe at.
 * @param gap The distance in pixels from the bottom pipe to the top pipe.
 */
void draw_pipe_pair(int x, int y, int gap) {
  // Draw the bottom pipe.
  draw_pipe(x, y, false);

  // Draw the top pipe. This pipe will be upside down, and
  // the Y coordinate will be the difference between the top
  // pipe's coordinate and the gap distance.
  draw_pipe(x, y-gap, true);
}


/**
 * Further abstraction to advance the pipes across the screen.
 * 
 * This function works to do four things:
 * 1. Create new pipes when needed.
 * 2. Move existing pipes across the screen.
 * 3. Update the score when a new pipe is created.
 * 4. Determine new random values for the gap distance and speed.
 */
void advance_pipes() {
  // First check to see if the X coordinate of the current pipe is
  // -15 or less. If it is, that means either the pipe is completely
  // off of the screen, or a new game has just begun.
  if (pipe[0] <= -15) { 
    // Set the X coordinate to the screen width. (Right edge of the screen)
    pipe[0] = SCREEN_WIDTH;

    // Determine random values for the Y coordinate, gap distance, and movement speed.
    pipe[1] = ESP8266TrueRandom.random(30, 58);
    pipe[2] = ESP8266TrueRandom.random(30, 50);
    advance = ESP8266TrueRandom.random(3, 10);

    // Increment the score
    score++;
  }
  
  // Draw the pipe (new or current)
  draw_pipe_pair(pipe[0], pipe[1], pipe[2]);
  // Advance the X coordinate to move the pipe
  pipe[0] = pipe[0] - advance;
}


/**
 * Function determines if the bird has collided with a pipe.
 * 
 * Given the current game variables, this function works to compute
 * whether or not two rectangles are overlapping. These rectangles are
 * basically an outline of the bird and the pipes.
 * 
 * In reality, it's a bit more complicated, and the function adjusts the
 * exact size of the rectangles to make the game a bit more lenient and 
 * visually accurate.
 * 
 * @param draw_rects Draws the calculated hitboxes if true. Only used for debugging.
 * @returns Boolean which is `true` if a collision has occurred.
 */
bool is_collision(bool draw_rects) {
  // Compute hitboxes for the bird...
  Point bird1 = {3, (bird_height+3)};
  Rect bird = {bird1, 17, 9};

  // ...the top pipe
  Point pipe1_l = {pipe[0]+3, pipe[1]};
  Rect pipe1 = {pipe1_l, 15, 128 - pipe[1]};

  // ...and the bottom pipe.
  Point pipe2_l = {pipe[0]+3, 0};
  Rect pipe2 = {pipe2_l, 15, pipe[1] - pipe[2]};

  // This just draws the rectangles for debugging, if enabled.
  if (draw_rects) {
    display.drawRect(bird.p1.x, bird.p1.y, bird.w, bird.h, 1);
    display.drawRect(pipe1.p1.x, pipe1.p1.y, pipe1.w, pipe1.h, 1);
    display.drawRect(pipe2.p1.x, pipe2.p1.y, pipe2.w, pipe2.h, 1);
  }

  // One of two conditions will be true if a collision has occurred:
  // Either the bird collided with the top pipe, or the bottom pipe.
  // If neither of those two are true, then a collision has not occurred.
  return rect_overlap(bird, pipe1) || rect_overlap(bird, pipe2);
}


/**
 * Draw a single frame of animation.
 * 
 * This function is ultimately the game's internal loop.
 */
void draw_frame() {
  // Start by completely clearing the display.
  display.clearDisplay();

  // Draw the bird
  draw_bird(bird_height);
  // Advance the position of the pipes
  advance_pipes();
  
  // See if a collision has occurred.
  if (is_collision(false)) {
    // If so, you lose!
    game_over();
  } else {
    // Otherwise, display the output, then delay
    // for the frame period.
    display.display();
    delay(1000 / FRAMERATE);
  }
}


/**
 * Setup function.
 * 
 * In Arduino architecture, the setup function is performed
 * once when the system first powers on. All the stuff done
 * here is just staging to make sure the system is ready
 * to run the program.
 */
void setup() {
  // Initialize buttons.
  a_button.begin();
  rst_button.begin();

  // Begin EEPROM storage for high score
  EEPROM.begin(512);

  // Initialize display.
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  display.clearDisplay();
  display.display();
  // Wait for 200 ms so display can finish up.
  delay(200);

  // Here we check for  both the A button and Reset buttons to be
  // held, but this check is specifically only performed during the
  // setup function.
  // The tl;dr of this is that if both buttons are held when the device
  // is powered on, this will run. It allows us to have a sneaky but
  // not too easy to trip method of resetting the high scores.
  if (a_button.read() && rst_button.read()) {
    set_high_score(0);
    display.setTextSize(1);
    display.setCursor(16, 16);
    display.setTextColor(WHITE);
    display.println("Scores Reset");
    display.println("   Press A...");
    display.display();
    while (a_button.read() && rst_button.read()) {
      delay(100);
    }
    a_button.await_click();
  }
}


/**
 * Loop function.
 * 
 * In the Arduino architecture, the loop function is
 * run continuously forever. (Or at least, until power
 * is removed.)
 * 
 * As all games are essentially loops, this sets off the
 * core functionality of the game, and is responsible for
 * advancing frames.
 */
void loop() {
  // If the game hasn't started, it must mean that the
  // system just booted.
  if (game_started == false) {
    // Display the title screen...
    title_screen();
    // ...wait for the A button to be pressed...
    a_button.await_click();
    // ...and then set the game to be started.
    game_started = true;
  }
  
  // Draw a frame.
  draw_frame();

  // Wait for the user to press the A button
  while (game_started == false && !a_button.read()) {
    // Delay for 1 ms until they do.
    delay(1);
  }

  // Set game to be started.
  game_started = true;

  // If the button is being pressed and the bird is not
  // at the top of the screen, increase the bird's height.
  if (a_button.read() && bird_height > 0) {
    // Note that because we're in quadrant IV, decreasing the
    // Y value has the action of "increasing" the bird's height.
    bird_height = bird_height - 3;

  // If the button isn't being pressed and the bird isn't
  // at the bottom of the screen, decrease the bird's height.
  } else if (bird_height < 52) {
    bird_height = bird_height + 2;
  }

  // Do all of this forever.
}