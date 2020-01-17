/*
  ByteBuffer.h - A circular buffer implementation for Arduino
  Created by Sigurdur Orn, July 19, 2010.  siggi@mit.edu
  Updated by GreyGnome (aka Mike Schwager) Thu Feb 23 17:25:14 CST 2012
        added the putString() method and the fillError variable.
        added the checkError() and resetError() methods.  The checkError() method resets the fillError variable
        to false as a side effect.
        added the ByteBuffer(unsigned int buf_size) constructor.
        added the init() method, and had the constructor call it automagically.
        protected certain sections of the code with cli()/sei() calls, for safe use by interrupts.
        Also made the capacity, position, length, and fillError variables volatile, for safe use by interrupts.
 Updated by Andrew J. Kroll
        Renamed library and removed the AVR bits and ISR protection,
        because this will only be used in ISR safe code.
        Might kill methods ither than standard ones if they prove to be useless.
 */

#ifndef UHS_ByteBuffer_h
#define UHS_ByteBuffer_h

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define DEFAULTBUFSIZE 32

class UHS_ByteBuffer {
public:

        UHS_ByteBuffer() {
                init();
        };

        UHS_ByteBuffer(unsigned int buf_size) {
                init(buf_size);
        };

        // This method initializes the datastore of the buffer to a certain size.
        void init(unsigned int buf_size);

        // This method initializes the datastore of the buffer to the default size.
        void init();

        // This method resets the buffer into an original state (with no data)
        void clear();

        // This method resets the fillError variable to false.
        void resetError();

        // This method tells you if your buffer overflowed at some time since the last
        // check.  The error state will be reset to false.
        bool checkError();

        // This releases resources for this buffer, after this has been called the buffer should NOT be used
        void deAllocate();

        // Returns how much space is used in the buffer
        int getSize();

        // Returns how much space is available
        int AvailableForPut();

        // Returns the maximum capacity of the buffer
        int getCapacity();

        // This method returns the byte that is located at index in the buffer but doesn't modify the buffer like the get methods (doesn't remove the retured byte from the buffer)
        uint8_t peek(unsigned int index);

        //
        // Put methods, either a regular put in back or put in front
        //
        uint8_t putInFront(uint8_t in);
        uint8_t put(uint8_t in);
        uint8_t putString(const char *in);
        uint8_t putString(char *in);

        void putIntInFront(int in);
        void putInt(int in);

        void putHex(uint8_t in);
        void putDec(uint8_t in);
        void putDec(int8_t in);

        void putLongInFront(long in);
        void putLong(long in);

        void putFloatInFront(float in);
        void putFloat(float in);

        //
        // Get methods, either a regular get from front or from back
        //
        uint8_t get();
        uint8_t getFromBack();

        int getInt();
        int getIntFromBack();

        long getLong();
        long getLongFromBack();

        float getFloat();
        float getFloatFromBack();

private:
        uint8_t* data=NULL;

        volatile unsigned int capacity=0;
        volatile unsigned int position=0;
        volatile unsigned int length=0;
        volatile bool fillError;
};

#endif
