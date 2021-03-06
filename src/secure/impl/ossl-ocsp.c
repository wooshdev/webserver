/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * TODO: Improve code documentation.
 */
#include <openssl/ocsp.h>
#include <openssl/bio.h>
#include <openssl/safestack.h>

typedef struct {
	/* (nullable) OCSP file. */
	char *file;
} ocsp_data_t;

unsigned char *rspder = NULL;
int rspderlen;

/*
 * Certificate Status callback. This is called when a client includes a
 * certificate status request extension. The response is either obtained from a
 * file, or from an OCSP responder.
 */
static int cert_status_cb(SSL *s, void *arg) {
	SSL_set_tlsext_status_ocsp_resp(s, rspder, rspderlen);
	return SSL_TLSEXT_ERR_OK;
}

int setup_ocsp(ocsp_data_t *data) {
    OCSP_RESPONSE *resp = NULL;
    int ret = 0;

    if (data->file) {
		FILE *file = fopen(data->file, "r");
		if (!file) {
			perror("[OCSP] Failed to open OCSP file");
			goto err;
		}
		
		BIO *derbio = BIO_new(BIO_s_file());
        
        if (!BIO_set_fp(derbio, file, BIO_NOCLOSE)) {
			fclose(file);
			BIO_free(derbio);
            fputs("cert_status: Cannot open OCSP response file\n", stderr);
			printf("ocsp file: %s\n", data->file);
			ERR_print_errors_fp(stderr);
            goto err;
        }
        resp = d2i_OCSP_RESPONSE_bio(derbio, NULL);
        BIO_free(derbio);
		fclose(file);
        if (resp == NULL) {
            fputs("cert_status: Error reading OCSP response\n", stderr);
            goto err;
        }
    } else {
		fputs("OCSP file not set.\n", stderr);
	}

    rspderlen = i2d_OCSP_RESPONSE(resp, &rspder);
    if (rspderlen <= 0)
        goto err;
	
	/*puts("cert_status: ocsp response sent:");
    OCSP_RESPONSE_print(bio_err, resp, 2);*/

    ret = SSL_TLSEXT_ERR_OK;
	goto end;
err:
	ret = 0;
end:
    OCSP_RESPONSE_free(resp);
    return ret;
} 
