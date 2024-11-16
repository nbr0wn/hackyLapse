/**	wifi-captive-portal-esp-idf-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.

  Contains some modified example code from here:
  https://github.com/espressif/esp-idf/blob/release/v4.2/examples/protocols/http_server/simple/main/main.c
  https://github.com/espressif/esp-idf/blob/release/v4.2/examples/protocols/http_server/restful_server/main/rest_server.c

  Original Example Code Header:
  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_vfs.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_http_server.h"
#include "wifi-captive-portal-esp-idf-httpd.h"

#include "control.h"

static char mainPage[] = R"rawliteral(
<!DOCTYPE html>
<html>
<title>HackyLapse</title>

<head>
  <style>
    body {
      background-color: #122138;
      font-size: 10vw;
    }

    a {
      text-decoration: none;
    }

    hr {
      margin: 0 0 10px 10px;
    }


    h1 {
      font-family: Arial, Helvetica, sans-serif;
      text-align: center;
      white-space: nowrap;
      color: #5d779e;
      font-size: 10vw;
      margin: 50px;
    }

    h2 {
      font-family: Arial, Helvetica, sans-serif;
      text-align: center;
      white-space: nowrap;
      margin: 0;
      color: #30ee30;
      font-size: 10vw;
    }

    .centered {
      display: flex;
      justify-content: center;
    }

    .button {
      margin: 0px 20px 0px 20px
    }

    .timer {
      color: white;
    }

    .big {
      width: fit-content;
      display: inline-block;
    }

    .titlefirst {
      color: slate;
    }

    .titlesecond {
      color: lightblue;
    }
  </style>
</head>

<body>

  <div class="centered">
    <h1><span class="titlefirst">Hacky</span><span class="titlesecond">Lapse</span></h1>
  </div>

  <div class="centered">
    <div class="big">
      <h2>TIMEOUT:</h2>
    </div>
    <div class="big timer">
      <h2><span id="timeoutval">XXXXXXXX</span>ms</h2>
    </div>
  </div>
  <div class="centered">
    <div class="button">
      <a href="#" onclick="setTime(5000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">5s</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="setTime(10000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">10s</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="setTime(20000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">20s</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="setTime(30000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">30s</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
  </div>
  <div class="centered">
    <div class="button">
      <a href="#" onclick="setTime(60000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">1m</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="setTime(300000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">5m</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="setTime(600000)">
        <svg width="128px" height="128px" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="128px" height="128px" rx="30" transform="translate(0 0)" fill="green" stroke="#green"
              stroke-width="1"></rect>
            <text transform="translate(64 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">10m</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
  </div>
  <div class="centered">
    <div class="button">
      <a href="#" onclick="addTime(500)">
        <svg width="200" height="128px" viewBox="0 0 200 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="200px" height="128px" rx="30" transform="translate(0 0)" fill="blue" stroke="#blue"
              stroke-width=""></rect>
            <text transform="translate(96 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">+500ms</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="addTime(-500)">
        <svg width="200" height="128px" viewBox="0 0 200 128" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink">
          <g transform="translate(0 0)">
            <rect width="200px" height="128px" rx="30" transform="translate(0 0)" fill="blue" stroke="#blue"
              stroke-width=""></rect>
            <text transform="translate(96 64)" fill="white" font-size="44" text-anchor="middle"
              font-family="Verdana, Arial, Helvetica, sans-serif">
              <tspan x="0" y="0" dy="+.3em">-500ms</tspan>
            </text>
          </g>
        </svg>
      </a>
    </div>
  </div>
  <hr>
  <div class="centered">
    <div class="button">
      <a href="#" onclick="startStop(1)">
        <svg version="1.1" width="128px" height="128px" viewBox="0 0 36 36" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink" aria-hidden="true" role="img" class="iconify iconify--twemoji"
          preserveAspectRatio="xMidYMid meet">
          <path fill="#00FF00" d="M36 32a4 4 0 0 1-4 4H4a4 4 0 0 1-4-4V4a4 4 0 0 1 4-4h28a4 4 0 0 1 4 4v28z"></path>
          <path fill="#FFF" d="M8 7l22 11L8 29z"></path>
        </svg>
      </a>
    </div>
    <div class="button">
      <a href="#" onclick="startStop(0)">
        <svg width="128px" height="128px" viewBox="0 0 36 36" xmlns="http://www.w3.org/2000/svg"
          xmlns:xlink="http://www.w3.org/1999/xlink" aria-hidden="true" role="img" class="iconify iconify--twemoji"
          preserveAspectRatio="xMidYMid meet">
          <path fill="#FF0000" d="M36 32a4 4 0 0 1-4 4H4a4 4 0 0 1-4-4V4a4 4 0 0 1 4-4h28a4 4 0 0 1 4 4v28z"></path>
          <path fill="#FFF" d="M20 7h5v22h-5zm-9 0h5v22h-5z"></path>
        </svg>
      </a>
    </div>
  </div>
  <hr>
  <div class="centered">
    <div class="button">
      <a href="#" onclick="snapshot()">
        <svg fill="#000000" xmlns="http://www.w3.org/2000/svg" width="128px" height="128px" viewBox="0 0 52 52"
          enable-background="new 0 0 52 52" xml:space="preserve">
          <g>
            <path fill="#fff" d="M26,20c-4.4,0-8,3.6-8,8s3.6,8,8,8s8-3.6,8-8S30.4,20,26,20z" />
            <path fill="#fff" d="M46,14h-5.2c-1.4,0-2.6-0.7-3.4-1.8l-2.3-3.5C34.4,7,32.7,6,30.9,6h-9.8c-1.8,0-3.5,1-4.3,2.7l-2.3,3.5
          c-0.7,1.1-2,1.8-3.4,1.8H6c-2.2,0-4,1.8-4,4v24c0,2.2,1.8,4,4,4h40c2.2,0,4-1.8,4-4V18C50,15.8,48.2,14,46,14z M26,40
          c-6.6,0-12-5.4-12-12s5.4-12,12-12s12,5.4,12,12S32.6,40,26,40z" />
          </g>
        </svg>
      </a>
    </div>
  </div>
  <script>
    function getTime() {
    var request = new XMLHttpRequest();
    request.open('GET', '/timeoutval', true);
    // this following line is needed to tell the server this is a ajax request
    request.setRequestHeader("X-Requested-With", "XMLHttpRequest");
    request.onload = function () {
    if (this.status >= 200 && this.status < 400) {
            var json = JSON.parse(this.response);
            const timeoutSpan = document.getElementById("timeoutval");
            timeoutSpan.textContent = json.timeoutval;
        }
    };
    request.send();
  }
  function addTime(val) {
    let xhr = new XMLHttpRequest();
    xhr.open('GET', `/?addtime=${val}`);
    xhr.send();
    getTime();
  }
  function setTime(val) {
    let xhr = new XMLHttpRequest();
    xhr.open('GET', `/?settime=${val}`);
    xhr.send();
    getTime();
  }
  function startStop(doit) {
    let xhr = new XMLHttpRequest();
    xhr.open('GET', `/?startstop=${doit}`);
    xhr.send();
    getTime();
  }
  function snapshot() {
    startStop(0);
    let xhr = new XMLHttpRequest();
    xhr.open('GET', "/snapshot");
    xhr.send();
  }
  </script>
</body>
</html>
)rawliteral";

#define REST_CHECK(a, str, goto_tag, ...)                                         \
  do                                                                              \
  {                                                                               \
    if (!(a))                                                                     \
    {                                                                             \
      ESP_LOGE(HTTPD_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
      goto goto_tag;                                                              \
    }                                                                             \
  } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

static const char *HTTPD_TAG = "wifi-captive-portal-esp-idf-httpd";

esp_event_loop_handle_t wifi_captive_portal_esp_idf_httpd_event_loop_handle;

ESP_EVENT_DEFINE_BASE(WIFI_CAPTIVE_PORTAL_ESP_IDF_HTTPD_EVENT);

static const char *base_path = "/www";

typedef struct rest_server_context
{
  char base_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
  size_t req_hdr_host_len = httpd_req_get_hdr_value_len(req, "Host");

  char req_hdr_host_val[req_hdr_host_len + 1];

  esp_err_t res = httpd_req_get_hdr_value_str(req, "Host", (char *)&req_hdr_host_val, sizeof(char) * req_hdr_host_len + 1);
  if (res != ESP_OK)
  {
    ESP_LOGE(HTTPD_TAG, "failed getting HOST header value: %d", res);

    switch (res)
    {
    case ESP_ERR_NOT_FOUND:
      ESP_LOGE(HTTPD_TAG, "failed getting HOST header value: ESP_ERR_NOT_FOUND: Key not found: %d", res);
      break;

    case ESP_ERR_INVALID_ARG:
      ESP_LOGE(HTTPD_TAG, "failed getting HOST header value: ESP_ERR_INVALID_ARG: Null arguments: %d", res);
      break;

    case ESP_ERR_HTTPD_INVALID_REQ:
      ESP_LOGE(HTTPD_TAG, "failed getting HOST header value: ESP_ERR_HTTPD_INVALID_REQ: Invalid HTTP request pointer: %d", res);
      break;

    case ESP_ERR_HTTPD_RESULT_TRUNC:
      ESP_LOGE(HTTPD_TAG, "failed getting HOST header value: ESP_ERR_HTTPD_RESULT_TRUNC: Value string truncated: %d", res);
      break;

    default:
      break;
    }
  }

  ESP_LOGI(HTTPD_TAG, "Got HOST header value: %s", req_hdr_host_val);

  const char redir_trigger_host[] = "connectivitycheck.gstatic.com";

  if (strncmp(req_hdr_host_val, redir_trigger_host, strlen(redir_trigger_host)) == 0)
  {
    const char resp[] = "302 Found";

    ESP_LOGI(HTTPD_TAG, "Detected redirect trigger HOST: %s", redir_trigger_host);

    httpd_resp_set_status(req, resp);

    /** NOTE: This is where you redirect to whatever DNS address you prefer to open the
        captive portal page. This DNS address will be displayed at the top of the
        page, so maybe you want to choose a nice name to use (it can be any legal
        DNS name that you prefer. */
    httpd_resp_set_hdr(req, "Location", "http://wifi-captive-portal");

    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  }
  else
  {
    ESP_LOGI(HTTPD_TAG, "No redirect needed for HOST: %s", req_hdr_host_val);

    char *buf;
    size_t buf_len;

    /*	Get header value string length and allocate memory for length + 1,
        extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
      buf = malloc(buf_len);
      /* Copy null terminated value string into buffer */
      if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
      {
        ESP_LOGI(HTTPD_TAG, "Found header => Host: %s", buf);
      }
      free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1)
    {
      buf = malloc(buf_len);
      if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK)
      {
        ESP_LOGI(HTTPD_TAG, "Found header => Test-Header-2: %s", buf);
      }
      free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1)
    {
      buf = malloc(buf_len);
      if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK)
      {
        ESP_LOGI(HTTPD_TAG, "Found header => Test-Header-1: %s", buf);
      }
      free(buf);
    }

    /*	Read URL query string length and allocate memory for length + 1,
        extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
      buf = malloc(buf_len);
      if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
      {
        ESP_LOGI(HTTPD_TAG, "Found URL query => %s", buf);
        char param[32];
        /* Get value of expected key from query string */
        if (httpd_query_key_value(buf, "addtime", param, sizeof(param)) == ESP_OK)
        {
          int time = atoi(param);
          ESP_LOGI(HTTPD_TAG, "ADD TIME: %d ms", time);
          addTime(time);
        }
        if (httpd_query_key_value(buf, "setTime", param, sizeof(param)) == ESP_OK)
        {
          int time = atoi(param);
          ESP_LOGI(HTTPD_TAG, "SET TIME: %d ms", time);
          setTime(time);
        }
        if (httpd_query_key_value(buf, "startstop", param, sizeof(param)) == ESP_OK)
        {
          int startstop = atoi(param);
          ESP_LOGI(HTTPD_TAG, "STARTSTOP: %d", startstop);
          if(startstop){
            setState(STATE_RUNNING);
          } else {
            setState(STATE_PAUSED);
          }
        }
      }
      free(buf);
    }


    // Templateish ugly HACK - replace fixed string with numeric value
    char msVal[32];
    sprintf(msVal,"%8d", getShutterTimeout());
    ESP_LOGI(HTTPD_TAG, " TIMEOUT %s", msVal);
    char *newBuf = (char *)malloc(strlen(req->user_ctx));
    memset(newBuf,0,strlen(req->user_ctx));
    memcpy(newBuf,req->user_ctx, strlen(req->user_ctx));
    char *loc = strstr(req->user_ctx, "XXXXXXXX");
    if(loc) {
      memcpy(loc,msVal,8);
    }
    httpd_resp_send(req, newBuf, strlen(newBuf));
    free(newBuf);

    /*	Send response with custom headers and body set as the
        string passed in user context */
    //const char *resp_str = (const char *)req->user_ctx;
    //httpd_resp_send(req, resp_str, strlen(resp_str));

    /*	After sending the HTTP response the old HTTP request
        headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0)
    {
      ESP_LOGI(HTTPD_TAG, "Request headers lost");
    }
  }

  return ESP_OK;
}

static esp_err_t timeout_get_handler(httpd_req_t *req) {
    char *newBuf = (char *)malloc(128);
    sprintf(newBuf, "{\"timeoutval\":%d}", getShutterTimeout());
    httpd_resp_send(req, newBuf, strlen(newBuf));
    free(newBuf);
    return ESP_OK;
}

static esp_err_t snapshot_get_handler(httpd_req_t *req) {
    char *newBuf = (char *)malloc(128);
    sprintf(newBuf, "200 OK");
    httpd_resp_send(req, newBuf, strlen(newBuf));
    free(newBuf);
    setState(STATE_SNAPSHOT);
    return ESP_OK;
}

static void start_httpd(void *pvParameter)
{
  /** HTTP server */
  ESP_LOGI(HTTPD_TAG, "Starting HTTP Server...");

  REST_CHECK(base_path, "wrong base path", err);
  rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
  REST_CHECK(rest_context, "No memory for rest context", err);
  strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

  httpd_handle_t server = NULL;

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.lru_purge_enable = true;

  REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start HTTP server failed", err_start);
  ESP_LOGI(HTTPD_TAG, "Started HTTP Server.");

  ESP_LOGI(HTTPD_TAG, "Registering HTTP server URI handlers...");

  // URI handlers

  httpd_uri_t timeoutval_get_uri = {
      .uri = "/timeoutval",
      .method = HTTP_GET,
      .handler = timeout_get_handler,
      .user_ctx = 0};

  httpd_register_uri_handler(server, &timeoutval_get_uri);

  httpd_uri_t snapshot_get_uri = {
      .uri = "/snapshot",
      .method = HTTP_GET,
      .handler = snapshot_get_handler,
      .user_ctx = 0};

  httpd_register_uri_handler(server, &snapshot_get_uri);

  httpd_uri_t common_get_uri = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = rest_common_get_handler,
      .user_ctx = mainPage};

  httpd_register_uri_handler(server, &common_get_uri);


  ESP_LOGI(HTTPD_TAG, "Registered HTTP server URI handlers.");

  return;

err_start:
  free(rest_context);

err:
  return;
}

void wifi_captive_portal_esp_idf_httpd_task(void *pvParameter)
{
  while (1)
  {
    start_httpd(NULL);

    /** TODO: xEventGroupWaitBits or similar might be much better than vTaskDelay for this section. */
    while (1)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
  }
}
