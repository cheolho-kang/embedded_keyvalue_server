#ifndef __UTIL_C___H_
#define __UTIL_C___H_

#include <dirent.h>
#include <unistd.h>
// #include "lrucache.h"
#include "server.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

void *LRUCacheCreate();
void LRUCacheDestroy(void *mlru);
int LRUCacheInsert(void *mlru, char *_key, struct hashEntry *_val);
struct hashEntry *LRUCacheGet(void *mlru, char *_key);
struct hashEntry *LRUCacheEvict(void *mlru);
void LRUCacheErase(void *mlru, char *_key);

void *QueueCreate();
void QueueDestroy(void *q);
void QueueEnqueue(void *mq, struct hashEntry *_val);
struct hashEntry *QueueDequeue(void *mq);
int QueueSize(void *mq);

void *ListCreate();
void ListPushFront(void *mq, struct hashEntry *_val);
void ListPushBack(void *mq, struct hashEntry *_val);
struct hashEntry *ListPopFront(void *mq);
struct hashEntry *ListPopBack(void *mq);
int ListSize(void *mq);

// FlashCache
void *flashCacheCreate(const char *_path, struct kvDb *db,
                       const uint64_t max_size, const uint32_t max_file_size);
void flashCacheDestroy(void *fc);
int flashCacheInsert(void *fc, const char *_key, const char *data,
                     const size_t size);
char *flashCacheLookup(void *fc, const char *_key, size_t *size);
int flashCacheErase(void *fc, const char *_key);
void flashCacheMetakeyDelete(void *fc, const char *_key);

#ifdef __cplusplus
}
#endif

#endif
