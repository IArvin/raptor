// 
// Copyright (c) 2018 DWCG-DWDO. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above 
//    copyright notice, this list of conditions and the following 
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its 
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Authros: Shadow Yuan(yxd@midu.com)
//

#pragma once
#include <utility>

namespace raptor {
template<typename T, int BlockSize = 4096>
class memory_pool {
    union Node_ {
        char element[sizeof(T)];  // T element;
        Node_* next;
    };
    typedef char*  data_pointer;
    typedef Node_  node_type;
    typedef Node_* node_pointer;

 public:
    typedef T* pointer_type;
    template<typename _Other>
    struct rebind {
        typedef memory_pool<_Other> other;
    };

    memory_pool() noexcept {
        current_block_list_ = nullptr;
        current_node_ = nullptr;
        last_node_ = nullptr;
        free_node_list_ = nullptr;
    }

    ~memory_pool() noexcept {
        node_pointer block = current_block_list_;
        while (block != nullptr) {
            node_pointer next = block->next;
            operator delete(reinterpret_cast<void*>(block));
            block = next;
        }
    }
    
    pointer_type allocate() {
        if (free_node_list_ != nullptr) {
            pointer_type result = reinterpret_cast<pointer_type>(free_node_list_);
            free_node_list_ = free_node_list_->next;
            return result;
        }
        
        if (current_node_ >= last_node_) {
            data_pointer block = reinterpret_cast<data_pointer>(operator new(BlockSize));
            reinterpret_cast<node_pointer>(block)->next = current_block_list_;
            current_block_list_ = reinterpret_cast<node_pointer>(block);
            data_pointer body = block + sizeof(node_pointer);
            
            // |<------------------------------- BlockSize ------------------------------->|
            // |<--next pointer-->|<--padding-->|<--Node 1-->|<--Node 2-->|...|<--Node n-->|
            // |                  next pointer = sizeof(node_pointer)                      |
            // |                  padding length < sizeof(Node)                            |
            // |<------------------------------------------------------------------------->|

            // (BlockSize - sizeof(node_pointer)) 
            // leave a pointer length for forming a linked list
            size_t padding = (BlockSize - sizeof(node_pointer)) % sizeof(node_type);
            current_node_ = reinterpret_cast<node_pointer>(body + padding);
            last_node_ = reinterpret_cast<node_pointer>(block + BlockSize - sizeof(node_type) + 1);
        }
        return reinterpret_cast<pointer_type>(current_node_++);
    }
    
    void deallocate(pointer_type ptr) {
        if (ptr != nullptr) {
            reinterpret_cast<node_pointer>(ptr)->next = free_node_list_;
            free_node_list_ = reinterpret_cast<node_pointer>(ptr);
        }
    }
    
    template<typename _Type, typename... Args>
    void construct(_Type* ptr, Args&&... args) {
        new (reinterpret_cast<void*>(ptr)) _Type(std::forward<Args>(args)...);
    }
    
    template<typename _Type, typename... Args>
    void destory(_Type* ptr) {
        ptr->~_Type();
    }

 private:
    node_pointer current_block_list_;  // list
    node_pointer current_node_;
    node_pointer last_node_;
    node_pointer free_node_list_;  // list

    static_assert(BlockSize >= 2 * sizeof(node_type), "[memory pool] BlockSize too small.");
};
}  // namespace raptor
