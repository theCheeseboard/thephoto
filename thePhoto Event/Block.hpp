//
//  Block.hpp
//  thePhoto Event
//
//  Created by Victor Tran on 13/12/18.
//  Copyright © 2018 Victor Tran. All rights reserved.
//

#ifndef Block_hpp
#define Block_hpp

#include <stdio.h>

struct BlockListPrivate;
struct BlockObjC;

struct Block {
    Block();
    ~Block();
    
    enum BlockType {
        Command,
        Image
    };
    
    BlockType type;
    BlockObjC* d;
};

class BlockList {
public:
    BlockList();
    ~BlockList();
    
    void pushBlock(Block b);
    Block takeBlock();
    unsigned long blockCount();
    
private:
    BlockListPrivate* d;
};

#endif /* Block_hpp */
