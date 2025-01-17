/****************************************************************************
 * modules/lte/altcom/api/mbedtls/rsa_init.c
 *
 *   Copyright 2018 Sony Corporation
 *   Copyright 2020 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dbg_if.h"
#include "altcom_errno.h"
#include "altcom_seterrno.h"
#include "apicmd_rsa_init.h"
#include "apiutil.h"
#include "ctx_id_mgr.h"
#include "mbedtls/rsa.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RSA_INIT_REQ_DATALEN (sizeof(struct apicmd_rsa_init_s))
#define RSA_INIT_RES_DATALEN (sizeof(struct apicmd_rsa_initres_s))

#define RSA_INIT_SUCCESS 0
#define RSA_INIT_FAILURE -1

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct rsa_init_req_s
{
  uint32_t id;
  int32_t  padding;
  int32_t  hash_id;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int32_t rsa_init_request(FAR struct rsa_init_req_s *req)
{
  int32_t                         ret;
  uint16_t                        reslen = 0;
  FAR struct apicmd_rsa_init_s    *cmd = NULL;
  FAR struct apicmd_rsa_initres_s *res = NULL;

  /* Check ALTCOM protocol version */

  if (apicmdgw_get_protocolversion() != APICMD_VER_V1)
    {
      return RSA_INIT_FAILURE;
    }

  /* Allocate send and response command buffer */

  if (!altcom_mbedtls_alloc_cmdandresbuff(
    (FAR void **)&cmd, APICMDID_TLS_RSA_INIT, RSA_INIT_REQ_DATALEN,
    (FAR void **)&res, RSA_INIT_RES_DATALEN))
    {
      return RSA_INIT_FAILURE;
    }

  /* Fill the data */

  cmd->ctx = htonl(req->id);
  cmd->padding = htonl(req->padding);
  cmd->hash_id = htonl(req->hash_id);

  DBGIF_LOG1_DEBUG("[rsa_init]ctx id: %d\n", req->id);
  DBGIF_LOG1_DEBUG("[rsa_init]padding: %d\n", req->padding);
  DBGIF_LOG1_DEBUG("[rsa_init]hash_id: %d\n", req->hash_id);

  /* Send command and block until receive a response */

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res,
                      RSA_INIT_RES_DATALEN, &reslen,
                      SYS_TIMEO_FEVR);

  if (ret < 0)
    {
      DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
      goto errout_with_cmdfree;
    }

  if (reslen != RSA_INIT_RES_DATALEN)
    {
      DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
      goto errout_with_cmdfree;
    }

  ret = ntohl(res->ret_code);

  DBGIF_LOG1_DEBUG("[rsa_init res]ret: %d\n", ret);

  altcom_mbedtls_free_cmdandresbuff(cmd, res);

  return RSA_INIT_SUCCESS;

errout_with_cmdfree:
  altcom_mbedtls_free_cmdandresbuff(cmd, res);
  return RSA_INIT_FAILURE;
}



/****************************************************************************
 * Public Functions
 ****************************************************************************/

void mbedtls_rsa_init(mbedtls_rsa_context *ctx, int padding, int hash_id)
{
  int32_t              result;
  struct rsa_init_req_s req;

  if (!altcom_isinit())
    {
      DBGIF_LOG_ERROR("Not intialized\n");
      altcom_seterrno(ALTCOM_ENETDOWN);
      return;
    }

  req.id = get_mbedtls_ctx_id(SSL_RSA_CTX);
  ctx->id = req.id;
  req.padding = padding;
  req.hash_id = hash_id;

  result = rsa_init_request(&req);

  if (result != RSA_INIT_SUCCESS)
    {
      DBGIF_LOG_ERROR("%s error.\n");
    }
}

