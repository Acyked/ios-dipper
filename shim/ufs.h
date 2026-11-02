#pragma once
#include <stdint.h>
void test_ufs(void);
int ufs_log_init(void);
void ulog_put(unsigned idx, uint64_t v);
