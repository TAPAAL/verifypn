/* 
 * File:   errorcodes.h
 * Author: Peter G. Jensen
 *
 * Created on 30 August 2016, 09:19
 */

#ifndef ERRORCODES_H
#define ERRORCODES_H

/** Enumeration of return values from VerifyPN */
enum ReturnValue {
    SuccessCode = 0,
    FailedCode = 1,
    UnknownCode = 2,
    ErrorCode = 3,
    ContinueCode = 4
};


#endif /* ERRORCODES_H */

