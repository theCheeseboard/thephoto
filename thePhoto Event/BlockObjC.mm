//
//  BlockObjC.mm
//  thePhoto Event
//
//  Created by Victor Tran on 13/12/18.
//  Copyright © 2018 Victor Tran. All rights reserved.
//

#include "BlockObjC.h"

Block::Block() {
    d = new BlockObjC();
}

Block::~Block() {
    delete d;
}

BlockObjC::BlockObjC() {
    
}

BlockObjC::~BlockObjC() {
    
}

Block BlockObjC::getCommand(NSString* command) {
    NSData* bytes = [[command stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding];
    
    Block b;
    b.d->data = bytes;
    
    return b;
}

Block BlockObjC::getImage(NSData* image) {
    return Block();
}

NSData* BlockObjC::getData() {
    return data;
}
