/*
  1.Our initial goal of designing this memory allocator is
    to make good use of the physical memory.
  2.Our implementation satisfies that the memory
      allocator has a utilization of 100%
  3.The design ensures that the kernel memory
  allocator controls its own address space.
*/
#include "mm_alloc.h"
#define align4(x) ((((x) - 1) >> 2) << 2) + 4
#include <stdlib.h>

void *baseAddr=NULL;

void split_block (s_block_ptr b, size_t s){
s_block_ptr new;
new = (s_block_ptr )(b->data+s);
new->size  =b->size - s - BLOCK_SIZE;
new->next = b->next;
new->prev = b;
new->free=1;
new->ptr= new->data;
b->size = s;
b->next = new;
if(new->next)
new->next->prev = new;
}
//Combining the blocks
s_block_ptr fusion(s_block_ptr b){
if(b->next && b->next->free){
b->size += BLOCK_SIZE + b->next->size;
b->next = b->next->next;
if(b->next)
b->next->prev = b;
}
return b;
}

s_block_ptr get_block (void *p){
char *b ;
b= p;
return (p=b -=BLOCK_SIZE);
}

s_block_ptr extend_heap (s_block_ptr last , size_t s){


s_block_ptr temp;
temp = sbrk(0);
size_t smoke;

smoke = (size_t)sbrk(BLOCK_SIZE+s);//set break to previous program break+s(bytes)
if(smoke<0){
return NULL;
}
//else
temp->size=s;
temp->next= NULL;
temp->prev= last;
temp->ptr = temp->data;
if(last){
last->next = temp;
temp->free = 0;// the new block is free

}
return temp;
}
void copy_block(s_block_ptr actual, s_block_ptr dust){
    int *data0, *data1;
    size_t size;
    data0 = actual->ptr;
    data1 = dust->ptr;
    for(size = 0; size * 4 < actual->size && size * 4 < dust->size; size++)
        data1[size] = data0[size];
}

s_block_ptr find_block(s_block_ptr last, size_t size){
    s_block_ptr temp = baseAddr;
    while (temp && !(temp->free && temp->size >= size)){
        last = temp;
        temp = temp->next;
    }
    return temp;
}

int valid_address(void *val){
    if (baseAddr){
        if(val > baseAddr && val < sbrk(0)) return (val == (get_block(val))->ptr);
    }
    return 0;
}

void* mm_malloc(size_t size)
{ size_t s;
  s_block_ptr temp, lastp;
   
    s = align4(size);
    if(baseAddr){
        lastp = baseAddr;
        temp = find_block(lastp, s);
        if(temp){
            if((temp->size - s) >= (BLOCK_SIZE + 4)) split_block(temp, s);
            temp->free = 0;
        }
        else{
            temp = extend_heap(lastp, s);
            if(!temp) return NULL;
        }
    }
    else{
      temp = extend_heap(NULL, s);
        if(!temp) return NULL;
        baseAddr = temp;
    }
    return temp->data;

} 

void* mm_realloc(void* ptr, size_t size)
{

 s_block_ptr temp, new;
size_t s;
    void *pointer;
    if(!ptr) return (malloc(size));
    if (valid_address(ptr)){
        s = align4(size);
        temp = get_block(ptr);
        if (temp->size >= s){
            if(temp->size - s >= (BLOCK_SIZE + 4)) split_block(temp, s);
        }
        else{
            if(temp->next && temp->next->free && (temp->size + BLOCK_SIZE + temp->next->size) >= s){
                fusion(temp);
                if(temp->size - s >= (BLOCK_SIZE + 4)) split_block(temp, s);
            }
            else{
                pointer = malloc(s);
                if (!pointer) return NULL;
                new = get_block(pointer);
                copy_block(temp, new);
                free(temp);
                return pointer;
            }
        }
        return ptr;
    }
    return NULL;

}

void mm_free(void* ptr)
{
    s_block_ptr b;
    if (valid_address(ptr)){
        b = get_block(ptr);
        b->free = 1;
        if(b->prev && b->prev->free) b = fusion(b->prev);
        if(b->next){
            fusion(b);
        }
        else{
            if(b->prev) b->prev->next = NULL;
            else baseAddr = NULL;
            brk(b);
        }
    }
}
