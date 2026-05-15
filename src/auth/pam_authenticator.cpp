#include "auth/pam_authenticator.h"

#include <cstring>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <string>

namespace noctalia::auth {
  namespace {
    struct ConversationData {
      const std::string* password = nullptr;
    };

    int converse(int count, const pam_message** messages, pam_response** responses, void* appdata) {
      if (count <= 0 || messages == nullptr || responses == nullptr) {
        return PAM_CONV_ERR;
      }

      auto* data = static_cast<ConversationData*>(appdata);
      auto* reply = static_cast<pam_response*>(calloc(static_cast<std::size_t>(count), sizeof(pam_response)));
      if (reply == nullptr) {
        return PAM_BUF_ERR;
      }

      for (int i = 0; i < count; ++i) {
        const int style = messages[i] != nullptr ? messages[i]->msg_style : PAM_ERROR_MSG;
        if (style == PAM_PROMPT_ECHO_OFF || style == PAM_PROMPT_ECHO_ON) {
          const char* text = data != nullptr && data->password != nullptr ? data->password->c_str() : "";
          reply[i].resp = strdup(text);
          if (reply[i].resp == nullptr) {
            free(reply);
            return PAM_BUF_ERR;
          }
        }
      }

      *responses = reply;
      return PAM_SUCCESS;
    }
  } // namespace

  PamResult PamAuthenticator::authenticate(const std::string& user, const std::string& password, const char* service) {
    ConversationData data{.password = &password};
    pam_conv conv{.conv = converse, .appdata_ptr = &data};
    pam_handle_t* pam = nullptr;

    int rc = pam_start(service, user.c_str(), &conv, &pam);
    if (rc != PAM_SUCCESS) {
      return {.ok = false, .error = pam_strerror(pam, rc), .environment = {}};
    }

    rc = pam_authenticate(pam, 0);
    if (rc == PAM_SUCCESS) {
      rc = pam_acct_mgmt(pam, 0);
    }
    if (rc == PAM_NEW_AUTHTOK_REQD) {
      rc = pam_chauthtok(pam, PAM_CHANGE_EXPIRED_AUTHTOK);
    }
    if (rc == PAM_SUCCESS) {
      rc = pam_open_session(pam, 0);
    }

    PamResult result;
    result.ok = rc == PAM_SUCCESS;
    if (!result.ok) {
      result.error = pam_strerror(pam, rc);
    } else if (char** env = pam_getenvlist(pam); env != nullptr) {
      for (char** it = env; *it != nullptr; ++it) {
        result.environment.emplace_back(*it);
        free(*it);
      }
      free(env);
    }

    pam_end(pam, rc);
    return result;
  }

} // namespace noctalia::auth
