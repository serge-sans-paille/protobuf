// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MEM_INTERNAL_ARENA_H_
#define UPB_MEM_INTERNAL_ARENA_H_

#include "upb/upb/mem/arena.h"

// Must be last.
#include "upb/upb/port/def.inc"

typedef struct _upb_MemBlock _upb_MemBlock;

struct upb_Arena {
  _upb_ArenaHead head;

  // upb_alloc* together with a low bit which signals if there is an initial
  // block.
  uintptr_t block_alloc;

  // When multiple arenas are fused together, each arena points to a parent
  // arena (root points to itself). The root tracks how many live arenas
  // reference it.

  // The low bit is tagged:
  //   0: pointer to parent
  //   1: count, left shifted by one
  UPB_ATOMIC(uintptr_t) parent_or_count;

  // All nodes that are fused together are in a singly-linked list.
  UPB_ATOMIC(upb_Arena*) next;  // NULL at end of list.

  // The last element of the linked list.  This is present only as an
  // optimization, so that we do not have to iterate over all members for every
  // fuse.  Only significant for an arena root.  In other cases it is ignored.
  UPB_ATOMIC(upb_Arena*) tail;  // == self when no other list members.

  // Linked list of blocks to free/cleanup.  Atomic only for the benefit of
  // upb_Arena_SpaceAllocated().
  UPB_ATOMIC(_upb_MemBlock*) blocks;
};

UPB_INLINE bool _upb_Arena_IsTaggedRefcount(uintptr_t parent_or_count) {
  return (parent_or_count & 1) == 1;
}

UPB_INLINE bool _upb_Arena_IsTaggedPointer(uintptr_t parent_or_count) {
  return (parent_or_count & 1) == 0;
}

UPB_INLINE uintptr_t _upb_Arena_RefCountFromTagged(uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(parent_or_count));
  return parent_or_count >> 1;
}

UPB_INLINE uintptr_t _upb_Arena_TaggedFromRefcount(uintptr_t refcount) {
  uintptr_t parent_or_count = (refcount << 1) | 1;
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(parent_or_count));
  return parent_or_count;
}

UPB_INLINE upb_Arena* _upb_Arena_PointerFromTagged(uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return (upb_Arena*)parent_or_count;
}

UPB_INLINE uintptr_t _upb_Arena_TaggedFromPointer(upb_Arena* a) {
  uintptr_t parent_or_count = (uintptr_t)a;
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return parent_or_count;
}

UPB_INLINE upb_alloc* upb_Arena_BlockAlloc(upb_Arena* arena) {
  return (upb_alloc*)(arena->block_alloc & ~0x1);
}

UPB_INLINE uintptr_t upb_Arena_MakeBlockAlloc(upb_alloc* alloc,
                                              bool has_initial) {
  uintptr_t alloc_uint = (uintptr_t)alloc;
  UPB_ASSERT((alloc_uint & 1) == 0);
  return alloc_uint | (has_initial ? 1 : 0);
}

UPB_INLINE bool upb_Arena_HasInitialBlock(upb_Arena* arena) {
  return arena->block_alloc & 0x1;
}

#include "upb/upb/port/undef.inc"

#endif /* UPB_MEM_INTERNAL_ARENA_H_ */
