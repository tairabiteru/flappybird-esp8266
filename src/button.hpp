/**
 * Module abstracting the concept of a button.
 * 
 * This contains simple shortcuts which the program
 * uses to read output from a button, including waiting
 * for a press, waiting for a press and hold, etc...
 */


#include <Arduino.h>

/**
 * Enum describing a Button's input modes.
*/
enum InputMode {
    /**
     * Normal mode - Button down = true, button up = false.
    */
    NORMAL,

    /**
     * Inverted mode - Button down = false, button up = true.
    */
    INVERTED
};


/**
 * Class abstracting the concept of a momentary switch on GPIO.
 * 
 * This class is quite specific for my use case to be honest. 
 * For example, it always initializes the button internally with
 * INPUT_PULLUP. Why? Because I'm lazy. :^)
 * 
 * Regardless, there's not much to understand here if you've got
 * some basic C++ knowledge, so I'll just leave it at that - again - 
 * because lazy. :^)
*/
class Button {
    public:
        int pin;
        InputMode mode;
    
        /**
         * Initialize a non-inverted button.
         * 
         * @param pin The GPIO pin to use for the button.
        */
        Button(int pin) {
            this->pin = pin;
            this->mode = InputMode::NORMAL;
        }

        /**
         * Initialize a button, either inverted or normal.
         * 
         * @param pin The GPIO pin to use for the button.
         * @param mode The InputMode to use for the button.
        */
        Button(int pin, InputMode mode) {
            this->pin = pin;
            this->mode = mode;
        }

        /**
         * Call the Arduino function pinMode() to initialize the button.
         * Again, this happens with INPUT_PULLUP only because lazy. :^)
        */
        void begin() {
            pinMode(this->pin, INPUT_PULLUP);
        }

        /**
         * Read the current state of the button.
         * 
         * This reads the button's 'actual' state. That is to say,
         * if the button is down it reads true, and if up, it reads false.
         * The inversion is done automatically according to the way the button
         * was initialized, so this will always return according to the above.
         * 
         * @return A boolean describing the state the button.
        */
        bool read() {
            if (this->mode == InputMode::INVERTED) {
                return !digitalRead(this->pin);
            }
            return digitalRead(this->pin);
        }

        /**
         * Shortcut method to block until the button is depressed.
        */
        void await_down() {
            while (!this->read()) {
                delay(1);
            }
        }

        /**
         * Shortcut method to block until the button is released.
        */
        void await_up() {
            while (this->read()) {
                delay(1);
            }
        }
        
        /**
         * Shortcut method to block until the button is pressed, then released.
        */
        void await_click() {
            this->await_down();
            this->await_up();
        }

        /**
         * Shortcut method to block until the button is pressed, held for an
         * amount of time, and then released.
         * 
         * @param ms The number of milliseconds to wait for.
        */
        void await_hold_and_release(unsigned long ms) {
            while (true) {
                this->await_down();
                unsigned long s = millis();
                this->await_up();
                unsigned long e = millis();

                if (e - s >= ms) {
                    break;
                }
            }
        }

        /**
         * Shortcut method to block until the button is held down for
         * an amount of time.
         * 
         * @param ms The number of milliseconds to wait for.
        */
        void await_hold_down(unsigned long ms) {
            this->await_up();

            while (true) {
                unsigned long s = millis();
                bool rtn = false;

                while (this->read()) {
                    if (millis() - s >= ms) {
                        rtn = true;
                        break;
                    }
                    delay(1);
                }

                if (rtn) {
                    break;
                }
                delay(1);
            }
        }
};