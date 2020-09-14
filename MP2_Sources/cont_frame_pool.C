/*
 File: ContFramePool.C
 
 Author: Cameron Bourque
 Date  : 09/13/2020
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

ContFramePool* ContFramePool::first = NULL;
ContFramePool* ContFramePool::last = NULL;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // Bitmap must fit in a single frame!
    assert(_n_frames <= FRAME_SIZE * 4);

    //add this to linked list
    if(first == NULL){
        first = this;
        last = this;
        prev = NULL;
        next = NULL;
    }
    else{
        prev = last; 
        next = NULL;
        last = this;
    }

    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;
    n_free_frames = _n_frames;

    if(info_frame_no == 0) {
        bitmap = (unsigned char *)(base_frame_no * (FRAME_SIZE / 2));
    }
    else {
        bitmap = (unsigned char *)(info_frame_no * (FRAME_SIZE / 2));
    }

    assert((n_frames % 4) == 0);

    for(unsigned long i = 0; i*4 < n_frames; i++){
        bitmap[i] = FREE;
    }

    if(info_frame_no == 0){
        for(unsigned long i = 0; i*4 < n_info_frames; i++){
            //determine if we need to mark head of sequence or not
            if(i == 0){
                bitmap[i] = HEAD_OF_SEQUENCE << 6;
            }
            else{
                bitmap[i] = ALLOCATED << 6;
            }
            n_free_frames--;
            
            //if last set of frames then check how many to fill
            if((i+1)*4 >= n_info_frames){
                unsigned long rem = n_info_frames % 4;
                if(rem == 0){
                    rem = 3;
                }
                else{
                    rem--;
                }
                unsigned char mask = ALLOCATED << 4;
                while(rem){
                    bitmap[i] |= mask;
                    mask >>= 2;
                    n_free_frames--;
                    rem--;
                }
            }
            else{
                unsigned char mask = ALLOCATED << 4;
                while(mask){
                    bitmap[i] |= mask;
                    mask >>= 2;
                    n_free_frames--;
                }
            }
        }
    }

    Console::puts("Frame pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Any frames left to allocate?
    assert(n_free_frames >= _n_frames);

    // Find a frame series length _n_frames that is not being used and return its frame index.
    // Mark that frame as being used in the bitmap.
    unsigned long frame_no = base_frame_no;

    unsigned long i = 0;
    unsigned char mask = 0xC0;
    while(i*4 < base_frame_no + n_frames){
        while((bitmap[i] & mask) && mask){
            mask >>= 2;
            frame_no++;
        }
        if(mask){
            assert(frame_no + _n_frames < base_frame_no + n_frames);
            unsigned long frame = frame_no;
            unsigned long bitmap_index = (frame - base_frame_no) / 4;
            unsigned int offset = ((frame - base_frame_no) % 4) * 2;
            //check whole sequence to see if it's free
            for(; frame < frame_no + _n_frames; frame++){
                if(bitmap[(frame - base_frame_no) / 4] &
                        (0xC0 >> (((frame - base_frame_no) % 4) * 2))){
                    break;
                }
            }
            //if sequence is open the allocate on it and return head frame
            if(frame == frame_no + _n_frames){
                bitmap[bitmap_index] |= HEAD_OF_SEQUENCE << (6 - offset);
                for(frame = frame_no + 1; frame < frame_no + _n_frames; frame++){
                    bitmap[(frame - base_frame_no) / 4] |= ALLOCATED << (6 - (((frame - base_frame_no) % 4) * 2));
                }
                return frame_no;
            }
        }

        //keep searching...
        i++;
        mask = 0xC0;
    }
    Console::puts("Error, unable to find sequence of ");
    Console::puti(_n_frames);
    Console::puts(" free frames\n");
    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    assert((_base_frame_no >= base_frame_no) && (_base_frame_no + _n_frames <= base_frame_no + n_frames));
    // Mark all frames in the range as being used.
    unsigned long i;
    for(i = _base_frame_no; i < _base_frame_no + _n_frames; i++){
        unsigned long bitmap_index = (i - base_frame_no) / 4;
        unsigned int offset = ((i - base_frame_no) % 4) * 2;
        unsigned char state = (i == _base_frame_no ? HEAD_OF_SEQUENCE : ALLOCATED) << (6 - offset);
        unsigned char mask = bitmap[bitmap_index] - (bitmap[bitmap_index] & (0xC0 >> offset));

        // Update bitmap
        bitmap[bitmap_index] = mask | state;
        n_free_frames--;
    }
}

void ContFramePool::release_frame_sequence(unsigned long _first_frame_no)
{
    unsigned long bitmap_index = (_first_frame_no - base_frame_no) / 4;
    unsigned int offset = ((_first_frame_no - base_frame_no) % 4) * 2;
    if((bitmap[bitmap_index] & (0xC0 >> offset)) == (HEAD_OF_SEQUENCE << (6 - offset))){
        unsigned char state = FREE << (6 - offset);
        unsigned char mask = bitmap[bitmap_index] - (bitmap[bitmap_index] & (0xC0 >> offset));
        bitmap[bitmap_index] = mask | state;
        n_free_frames++;
        
        for(unsigned long frame = _first_frame_no + 1;
                ((bitmap[(bitmap_index = (frame - base_frame_no) / 4)] &
                 (0xC0 >> (offset = ((frame - base_frame_no) % 4) * 2))) ==
                 (ALLOCATED << (6 - (((frame - base_frame_no) % 4) * 2)))) &&
                (frame < base_frame_no + n_frames);
                frame++){
            state = FREE << (6 - offset);
            mask = bitmap[bitmap_index] - (bitmap[bitmap_index] & (0xC0 >> offset));
            bitmap[bitmap_index] = mask | state;
            n_free_frames++;
        }
    }
    else{
        Console::puts("Error, first frame is not head of sequence\n");
        assert(false);
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    ContFramePool* iter = first;
    assert(iter != NULL);

    while(iter){
        if(_first_frame_no >= iter->base_frame_no && _first_frame_no < iter->base_frame_no + iter->n_frames){
            iter->release_frame_sequence(_first_frame_no);
            return;
        }
        else{
            iter = iter->next;
        }
    }
    Console::puts("Error, frame not in list");
    assert(false);
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    return (_n_frames / (FRAME_SIZE * 4)) + (_n_frames % (FRAME_SIZE * 4) > 0 ? 1 : 0);
}
