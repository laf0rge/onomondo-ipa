#pragma once

#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
struct asn_TYPE_descriptor_s;

#define IPA_LOGP_ES10B(func, level, fmt, args...) \
	IPA_LOGP(SES10B, level, "%s: " fmt, func, ## args)

#define IPA_ES10B_VERSION_LEN 3
#define IPA_ES10B_ASN_ENCODER_BUF_SIZE 5120

void *ipa_es10b_res_dec(const struct asn_TYPE_descriptor_s *td, const struct ipa_buf *es10b_res_encoded,
			const char *function_name);
struct ipa_buf *ipa_es10b_req_enc(const struct asn_TYPE_descriptor_s *td, const void *es10b_req_decoded,
				  const char *function_name);

/*! A helper macro to free the basic contents of an ES10B response. This macro is intended to be used from within the
 *  concrete implementation of an ES10B function. It only frees the common contents and the struct itsself. In case
 *  there are other additional fields, the caller must free those first before calling this macro.
 *  \param[in] res pointer struct that holds the ES10B response. */
#define IPA_ES10B_RES_FREE(td, res) ({		\
	if (!(res)) \
		return; \
	ASN_STRUCT_FREE(td, (res)->res); \
	IPA_FREE((res)); \
})
