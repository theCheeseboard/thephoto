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
#include <functional>

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
    
    void pushBlock(Block* b);
    Block* takeBlock();
    unsigned long blockCount();
    void clearBlocks();
    
    void setNewBlockAvailableCallback(std::function<void()> newBlockAvailableFunction);
    
private:
    BlockListPrivate* d;
    std::function<void()> newBlockAvailableFunction;
};

#endif /* Block_hpp */
