//
//  Block.cpp
//  thePhoto Event
//
//  Created by Victor Tran on 13/12/18.
//  Copyright © 2018 Victor Tran. All rights reserved.
//

#include "Block.hpp"
#include <list>

struct BlockListPrivate {
    std::list<Block*> blocks;
};

BlockList::BlockList() {
    d = new BlockListPrivate();
}

BlockList::~BlockList() {
    delete d;
}

void BlockList::pushBlock(Block* b) {
    d->blocks.push_back(b);
    newBlockAvailableFunction();
}

Block* BlockList::takeBlock() {
    Block* b = d->blocks.front();
    d->blocks.pop_front();
    return b;
}

unsigned long BlockList::blockCount() {
    return d->blocks.size();
}

void BlockList::setNewBlockAvailableCallback(std::function<void ()> newBlockAvailableFunction) {
    this->newBlockAvailableFunction = newBlockAvailableFunction;
}
