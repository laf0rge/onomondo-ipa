#pragma once

#define IPA_LOGP_ES10B(func, level, fmt, args...) \
	IPA_LOGP(SES10B, level, "%s: " fmt, func, ## args)

#define IPA_ES10B_VERSION_LEN 3
