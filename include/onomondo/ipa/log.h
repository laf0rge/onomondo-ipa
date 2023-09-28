#pragma once

#include <stdio.h>
#include <stdint.h>

#define IPA_LOGP(subsys, level, fmt, args...) \
	ipa_logp(subsys, level, __FILE__, __LINE__, fmt, ## args)

void ipa_logp(uint32_t subsys, uint32_t level, const char *file, int line,
	      const char *format, ...)
    __attribute__((format(printf, 5, 6)));

enum log_subsys {
	SMAIN,
	SHTTP,
	SSCARD,
	_NUM_LOG_SUBSYS
};

enum log_level {
	LERROR,
	LINFO,
	LDEBUG,
	_NUM_LOG_LEVEL
};
