#pragma once

#include <onomondo/ipa/utils.h>
#include <stddef.h>

/* \! Callback function to be passed to asn1c decoder function. */
int ipa_asn1c_consume_bytes_cb(const void *buffer, size_t size, void *priv);
