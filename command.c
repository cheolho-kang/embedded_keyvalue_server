#include "command.h"
int memUsedCheck(struct kvClient *c) {
  size_t mem_used = 0;
  mem_used = get_used_memory();
  // flush 시킬 object background에게 보내기
  int pool_size;
  if (server.cache_mode == MODE_MEM_FIFO)
    pool_size = ListSize(c->db->memList);
  else if (server.cache_mode == MODE_MEM_LRU)
    pool_size = LRUCacheSize(c->db->memLRU);
  if (mem_used > server.maxmemory * 90.0 / 100 && pool_size > 0) {
    struct hashEntry *he;
    char *evict_key = NULL;
    struct kvObject *valueObj;
    if (server.cache_mode == MODE_MEM_FIFO)
      he = ListPopBack(c->db->memList);
    else if (server.cache_mode == MODE_MEM_LRU)
      he = LRUCacheEvict(c->db->memLRU);
    valueObj = he->val;
    if (valueObj->where == KV_LOC_MEM) {
      evict_key = he->key;
      valueObj->where = KV_LOC_FLUSHING;
      QueueEnqueue(c->db->memQueue, he);
      sem_post(sem_bworker);
    }
  }

  mem_used = get_used_memory();
  if (mem_used < server.maxmemory) return 0;

  // 메모리 용량 초과
  if (mem_used >= server.maxmemory) {
    while (QueueSize(c->db->memQueue) > 0) {
    }
  }
}
int processCMD(struct kvClient *c) {
  if (msgcmp(c->argv[0]->ptr, "set")) {
    if (server.cache_mode != MODE_ONLY_FLASH) memUsedCheck(c);
    pthread_mutex_lock(&bworker_mutex);
    setCommand(c, c->argv[1], c->argv[2]);
    pthread_mutex_unlock(&bworker_mutex);
  } else if (msgcmp(c->argv[0]->ptr, "get")) {
    pthread_mutex_lock(&bworker_mutex);
    getCommand(c, c->argv[1]);
    pthread_mutex_unlock(&bworker_mutex);
  } else if (msgcmp(c->argv[0]->ptr, "scan")) {
  } else if (msgcmp(c->argv[0]->ptr, "kill")) {
    flashCacheDestroy(server.db->flashCache);
    // pid_t pid = getpid();
    // kill(pid, 2);
  } else if (msgcmp(c->argv[0]->ptr, "evict")) {
    evictAllCommand();
    // memUsedCheck(c);
  } else {
    char *msg = "+OK\r\n";
    c->querybuf->len = strlen(msg);
    memcpy(c->querybuf->buf, msg, c->querybuf->len);
    c->querybuf->buf[c->querybuf->len - 1] = 0;
  }
}
void cacheAdd(struct kvDb *db, struct kvObject *key, struct kvObject *val) {
  if (server.cache_mode == MODE_MEM_FIFO) {
    struct hashEntry *he = entryFind(db->memCache, key->ptr);
    if (he == NULL) {
      char *copy = msgdup((struct msg *)key->ptr);
      hashAdd(db->memCache, copy, val);
      ListPushFront(db->memList, he);
    } else {
      he->val = val;
    }
  } else if (server.cache_mode == MODE_MEM_LRU) {
    struct hashEntry *he = LRUCacheGet(db->memLRU, key->ptr->buf);
    if (he == NULL) {
      char *copy = msgdup((struct msg *)key->ptr);
      struct hashEntry *entry = chmalloc(sizeof(*entry));
      entry->key = copy;
      entry->val = val;
      LRUCacheInsert(db->memLRU, (char *)key->ptr->buf, entry);
    } else {
      he->val = val;
    }
  }
}
void setCommand(struct kvClient *c, struct kvObject *key,
                struct kvObject *val) {
  if (server.cache_mode == MODE_ONLY_FLASH) {
    flashCacheInsert(c->db->flashCache, (char *)key->ptr->buf,
                     (char *)val->ptr->buf, val->ptr->len);
  } else {
    cacheAdd(c->db, key, val);
  }
  char *msg = "+OK\r\n";
  c->querybuf->len = strlen(msg);
  memcpy(c->querybuf->buf, msg, c->querybuf->len);
  c->querybuf->buf[c->querybuf->len] = 0;
}

int getCommand(struct kvClient *c, struct kvObject *key) {
  char *ret_val = NULL;
  int val_size = 0;
  if (server.cache_mode == MODE_ONLY_FLASH) {
    if (ret_val =
            flashCacheLookup(c->db->flashCache, key->ptr->buf, &val_size)) {
      c->querybuf->len =
          sprintf(c->querybuf->buf, "$%d\r\n%s\r\n", val_size, ret_val);
    } else {
      c->querybuf->len = sprintf(c->querybuf->buf, "$7\r\n+Empty!\r\n");
    }
  } else {
    struct hashEntry *he;
    // 메모리에서 1차 검색
    if (server.cache_mode == MODE_MEM_FIFO) {
      he = entryFind(c->db->memCache, key->ptr);
    } else if (server.cache_mode == MODE_MEM_LRU) {
      he = LRUCacheGet(c->db->memLRU, (char *)key->ptr->buf);
    }
    // 메모리에서 찾음
    if (he) {
      c->querybuf->len = sprintf(c->querybuf->buf, "$%d\r\n%s\r\n",
                                 he->val->ptr->len, he->val->ptr->buf);
    } else {
      // flahs에서 2차 검색
      if (ret_val =
              flashCacheLookup(c->db->flashCache, key->ptr->buf, &val_size)) {
        // flashCache에서 찾은 값을 memory에 다시 업데이트
        struct kvObject *new_val = createObject(ret_val, val_size);
        cacheAdd(c->db, key, new_val);
        flashCacheMetakeyDelete(c->db->flashCache, key->ptr->buf);
        c->querybuf->len =
            sprintf(c->querybuf->buf, "$%d\r\n%s\r\n", val_size, ret_val);

      } else {
        // flash에서도 못찾음.
        c->querybuf->len = sprintf(c->querybuf->buf, "$7\r\n+Empty!\r\n");
      }
    }
  }
  return 0;
}
void evictAllCommand() {
  // flush 시킬 object background에게 보내기
  int pool_size;
  if (server.cache_mode == MODE_MEM_FIFO)
    pool_size = ListSize(server.db->memList);
  else if (server.cache_mode == MODE_MEM_LRU)
    pool_size = LRUCacheSize(server.db->memLRU);
  int i;
  // mem cache에 남아있는 엔트리 모두 background job에게 전달
  for (i = 0; i < pool_size; i++) {
    struct hashEntry *he;
    char *evict_key = NULL;
    struct kvObject *valueObj;
    if (server.cache_mode == MODE_MEM_FIFO)
      he = ListPopBack(server.db->memList);
    else if (server.cache_mode == MODE_MEM_LRU)
      he = LRUCacheEvict(server.db->memLRU);
    valueObj = he->val;
    if (valueObj->where == KV_LOC_MEM) {
      evict_key = he->key;
      valueObj->where = KV_LOC_FLUSHING;
      QueueEnqueue(server.db->memQueue, he);
      sem_post(sem_bworker);
    }
  }
  struct hash *mc = server.db->memCache;
  for (;;) {
    if (server.cache_mode == MODE_MEM_FIFO) {
      if (mc->ht[0].used == 0) break;
    } else if (server.cache_mode == MODE_MEM_LRU) {
      if (LRUCacheSize(server.db->memLRU) == 0) break;
    }
  }
  return;
}
