//
//  BlockObjC.h
//  thePhoto Event
//
//  Created by Victor Tran on 13/12/18.
//  Copyright © 2018 Victor Tran. All rights reserved.
//

#include "Block.hpp"
#include <string>
#import <UIKit/UIKit.h>

#ifndef BlockObjC_h
#define BlockObjC_h

class BlockObjC {
public:
    BlockObjC();
    ~BlockObjC();
    static Block* getCommand(NSString* command);
    static Block* getImage(NSData* image);
    
    NSData* getData();
    
private:
    NSData* data;
    NSUInteger len = 0;
};

#endif /* BlockObjC_h */
