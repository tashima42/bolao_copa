/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Brian J. Downs
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <papago.h>

static papago_t *server = NULL;

/**
 * Signal handler for graceful shutdown.
 */
static void signal_handler(int sig) {
  PAPAGO_UNUSED(sig);

  printf("\nShutting down...\n");
  if (server != NULL) {
    papago_stop(server);
  }
}
typedef struct {
  pthread_mutex_t mutex;
  FILE *file;
} shared_state_t;

// HTTP route handlers

static const char index_html[] =
    "<!DOCTYPE html>\n"
    "<html lang=\"pt-BR\">\n"
    "\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <meta name=\"viewport\" content=\"width=device-width, "
    "initial-scale=1.0\">\n"
    "  <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
    "  <title>Bolão da copa</title>\n"
    "  <style>\n"
    "    * {\n"
    "      box-sizing: border-box;\n"
    "      margin: 0;\n"
    "      padding: 0;\n"
    "    }\n"
    "\n"
    "    body {\n"
    "      background: #f0f2f5;\n"
    "      font-family: sans-serif;\n"
    "      padding: 32px 16px;\n"
    "      color: #333;\n"
    "    }\n"
    "\n"
    "    main {\n"
    "      max-width: 560px;\n"
    "      margin: 0 auto;\n"
    "    }\n"
    "\n"
    "    h1 {\n"
    "      font-size: 1.5rem;\n"
    "      margin-bottom: 24px;\n"
    "      text-align: center;\n"
    "    }\n"
    "\n"
    "    form {\n"
    "      display: flex;\n"
    "      flex-direction: column;\n"
    "      gap: 12px;\n"
    "    }\n"
    "\n"
    "    form>div:first-child {\n"
    "      background: #fff;\n"
    "      border-radius: 12px;\n"
    "      padding: 16px;\n"
    "      display: flex;\n"
    "      align-items: center;\n"
    "      gap: 12px;\n"
    "      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.08);\n"
    "    }\n"
    "\n"
    "    form>div:first-child label {\n"
    "      font-weight: 600;\n"
    "      white-space: nowrap;\n"
    "    }\n"
    "\n"
    "    form>div:first-child input {\n"
    "      flex: 1;\n"
    "      border: 1px solid #ddd;\n"
    "      border-radius: 8px;\n"
    "      padding: 8px 12px;\n"
    "      font-size: 1rem;\n"
    "      outline: none;\n"
    "      transition: border-color 0.2s;\n"
    "    }\n"
    "\n"
    "    form>div:first-child input:focus {\n"
    "      border-color: #4a90e2;\n"
    "    }\n"
    "\n"
    "    .match-wrapper {\n"
    "      background: #fff;\n"
    "      border-radius: 12px;\n"
    "      padding: 14px 16px;\n"
    "      display: flex;\n"
    "      align-items: center;\n"
    "      justify-content: space-between;\n"
    "      gap: 12px;\n"
    "      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.08);\n"
    "    }\n"
    "\n"
    "    .match-wrapper ul {\n"
    "      list-style: none;\n"
    "      font-size: 0.95rem;\n"
    "      line-height: 1.6;\n"
    "    }\n"
    "\n"
    "    .match-wrapper select {\n"
    "      border: 1px solid #ddd;\n"
    "      border-radius: 8px;\n"
    "      padding: 8px 12px;\n"
    "      font-size: 0.9rem;\n"
    "      background: #fafafa;\n"
    "      cursor: pointer;\n"
    "      outline: none;\n"
    "      transition: border-color 0.2s;\n"
    "      min-width: 150px;\n"
    "    }\n"
    "\n"
    "    .match-wrapper select:focus {\n"
    "      border-color: #4a90e2;\n"
    "    }\n"
    "\n"
    "    button[type=\"submit\"] {\n"
    "      margin-top: 8px;\n"
    "      padding: 14px;\n"
    "      background: #4a90e2;\n"
    "      color: #fff;\n"
    "      font-size: 1rem;\n"
    "      font-weight: 600;\n"
    "      border: none;\n"
    "      border-radius: 12px;\n"
    "      cursor: pointer;\n"
    "      transition: background 0.2s;\n"
    "    }\n"
    "\n"
    "    button[type=\"submit\"]:hover {\n"
    "      background: #357abd;\n"
    "    }\n"
    "  </style>\n"
    "</head>\n"
    "\n"
    "<body>\n"
    "  <main>\n"
    "    <h1>Bolão - Mata-mata 16 avos</h1>\n"
    "    <div class=\"matches\">\n"
    "      <form id=\"16-avos-matches\" action=\"/submit\" method=\"GET\">\n"
    "        <div>\n"
    "          <label for=\"name\">Nome:</label>\n"
    "          <input type=\"text\" id=\"name\" name=\"name\" required>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-1\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇿🇦 África do Sul</li>\n"
    "            <li>🇨🇦 Canadá</li>\n"
    "          </ul>\n"
    "          <select name=\"match-1\" id=\"match-1\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"africa-do-sul\">África do Sul</option>\n"
    "            <option value=\"canada\">Canadá</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-2\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇧🇷 Brasil</li>\n"
    "            <li>🇯🇵 Japão</li>\n"
    "          </ul>\n"
    "          <select name=\"match-2\" id=\"match-2\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"brasil\">Brasil</option>\n"
    "            <option value=\"japao\">Japão</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-3\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇩🇪 Alemanha</li>\n"
    "            <li>🇵🇾 Paraguai</li>\n"
    "          </ul>\n"
    "          <select name=\"match-3\" id=\"match-3\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"alemanha\">Alemanha</option>\n"
    "            <option value=\"paraguai\">Paraguai</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-4\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇳🇱 Holanda</li>\n"
    "            <li>🇲🇦 Marrocos</li>\n"
    "          </ul>\n"
    "          <select name=\"match-4\" id=\"match-4\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"holanda\">Holanda</option>\n"
    "            <option value=\"marrocos\">Marrocos</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-5\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇨🇮 Costa do Marfim</li>\n"
    "            <li>🇳🇴 Noruega</li>\n"
    "          </ul>\n"
    "          <select name=\"match-5\" id=\"match-5\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"costa-do-marfim\">Costa do Marfim</option>\n"
    "            <option value=\"noruega\">Noruega</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-6\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇫🇷 França</li>\n"
    "            <li>🇸🇪 Suécia</li>\n"
    "          </ul>\n"
    "          <select name=\"match-6\" id=\"match-6\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"franca\">França</option>\n"
    "            <option value=\"suecia\">Suécia</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-7\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇲🇽 México</li>\n"
    "            <li>🇪🇨 Equador</li>\n"
    "          </ul>\n"
    "          <select name=\"match-7\" id=\"match-7\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"mexico\">México</option>\n"
    "            <option value=\"equador\">Equador</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-8\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🏴󠁧󠁢󠁥󠁮󠁧󠁿 Inglaterra</li>\n"
    "            <li>🇨🇩 RD Congo</li>\n"
    "          </ul>\n"
    "          <select name=\"match-8\" id=\"match-8\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"inglaterra\">Inglaterra</option>\n"
    "            <option value=\"rd-congo\">RD Congo</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-9\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇧🇪 Bélgica</li>\n"
    "            <li>🇸🇳 Senegal</li>\n"
    "          </ul>\n"
    "          <select name=\"match-9\" id=\"match-9\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"belgica\">Bélgica</option>\n"
    "            <option value=\"senegal\">Senegal</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-10\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇺🇸 Estados Unidos</li>\n"
    "            <li>🇧🇦 Bósnia</li>\n"
    "          </ul>\n"
    "          <select name=\"match-10\" id=\"match-10\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"estados-unidos\">Estados Unidos</option>\n"
    "            <option value=\"bosnia\">Bósnia</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-11\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇪🇸 Espanha</li>\n"
    "            <li>🇦🇹 Áustria</li>\n"
    "          </ul>\n"
    "          <select name=\"match-11\" id=\"match-11\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"espanha\">Espanha</option>\n"
    "            <option value=\"austria\">Áustria</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-12\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇵🇹 Portugal</li>\n"
    "            <li>🇭🇷 Croácia</li>\n"
    "          </ul>\n"
    "          <select name=\"match-12\" id=\"match-12\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"portugal\">Portugal</option>\n"
    "            <option value=\"croacia\">Croácia</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-13\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇨🇭 Suíça</li>\n"
    "            <li>🇩🇿 Argélia</li>\n"
    "          </ul>\n"
    "          <select name=\"match-13\" id=\"match-13\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"suica\">Suíça</option>\n"
    "            <option value=\"argelia\">Argélia</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-14\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇦🇺 Austrália</li>\n"
    "            <li>🇪🇬 Egito</li>\n"
    "          </ul>\n"
    "          <select name=\"match-14\" id=\"match-14\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"australia\">Austrália</option>\n"
    "            <option value=\"egito\">Egito</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-15\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇦🇷 Argentina</li>\n"
    "            <li>🇨🇻 Cabo Verde</li>\n"
    "          </ul>\n"
    "          <select name=\"match-15\" id=\"match-15\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"argentina\">Argentina</option>\n"
    "            <option value=\"cabo-verde\">Cabo Verde</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <div id=\"wrapper-match-16\" class=\"match-wrapper\">\n"
    "          <ul>\n"
    "            <li>🇨🇴 Colômbia</li>\n"
    "            <li>🇬🇭 Gana</li>\n"
    "          </ul>\n"
    "          <select name=\"match-16\" id=\"match-16\" required>\n"
    "            <option value=\"\" disabled selected>Selecione</option>\n"
    "            <option value=\"colombia\">Colômbia</option>\n"
    "            <option value=\"gana\">Gana</option>\n"
    "          </select>\n"
    "        </div>\n"
    "        <button type=\"submit\">Enviar</button>\n"
    "      </form>\n"
    "    </div>\n"
    "  </main>\n"
    "  <script>\n"
    "    const params = new URLSearchParams(window.location.search);\n"
    "    const name = params.get('name');\n"
    "    if (name) document.getElementById('name').value = name;\n"
    "  </script>\n"
    "</body>\n"
    "\n"
    "</html>\n";

static const char submit_html[] =
    "<!DOCTYPE html>\n"
    "<html lang=\"pt-BR\">\n"
    "\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <meta name=\"viewport\" content=\"width=device-width, "
    "initial-scale=1.0\">\n"
    "  <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
    "  <title>Bolão da copa</title>\n"
    "</head>\n"
    "\n"
    "<body>\n"
    "  <main>\n"
    "    <h1>Obrigado!</h1>\n"
    "  </main>\n"
    "</body>\n"
    "\n"
    "</html>\n";

static bool logger_before(papago_request_t *req, papago_response_t *res,
                          void *user_data) {
  PAPAGO_UNUSED(req);
  PAPAGO_UNUSED(res);
  PAPAGO_UNUSED(user_data);

  return true;
}

static void logger_after(papago_request_t *req, papago_response_t *res,
                         void *user_data) {
  PAPAGO_UNUSED(user_data);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  double duration_ms =
      (now.tv_sec - papago_req_start_time(req).tv_sec) * 1000.0 +
      (now.tv_nsec - papago_req_start_time(req).tv_nsec) / 1.0e6;

  fprintf(stdout,
          "{\"remote\":\"%s\",\"method\":\"%s\",\"path\":\"%s\","
          "\"version\":\"%s\",\"host\":\"%s\",\"user_agent\":\"%s\","
          "\"query\":\"%s\","
          "\"status\":%d,\"duration_ms\":%.3f}\n",
          papago_req_client_ip(req) != NULL ? papago_req_client_ip(req) : "-",
          papago_req_method(req) != NULL ? papago_req_method(req) : "-",
          papago_req_path(req) != NULL ? papago_req_path(req) : "-",
          papago_req_version(req) != NULL ? papago_req_version(req) : "-",
          papago_req_host(req) != NULL ? papago_req_host(req) : "-",
          papago_req_user_agent(req) != NULL ? papago_req_user_agent(req) : "-",
          papago_req_query(req, NULL) != NULL ? papago_req_query(req, NULL)
                                              : "-",
          papago_res_status(res), duration_ms);
}

void index_handler(papago_request_t *req, papago_response_t *res,
                   void *user_data) {
  PAPAGO_UNUSED(req);
  PAPAGO_UNUSED(user_data);

  papago_res_send(res, index_html);
}

void submit_handler(papago_request_t *req, papago_response_t *res,
                    void *user_data) {
  shared_state_t *state = (shared_state_t *)user_data;

  const char *name = papago_req_query(req, "name");
  const char *match1 = papago_req_query(req, "match-1");
  const char *match2 = papago_req_query(req, "match-2");
  const char *match3 = papago_req_query(req, "match-3");
  const char *match4 = papago_req_query(req, "match-4");
  const char *match5 = papago_req_query(req, "match-5");
  const char *match6 = papago_req_query(req, "match-6");
  const char *match7 = papago_req_query(req, "match-7");
  const char *match8 = papago_req_query(req, "match-8");
  const char *match9 = papago_req_query(req, "match-9");
  const char *match10 = papago_req_query(req, "match-10");
  const char *match11 = papago_req_query(req, "match-11");
  const char *match12 = papago_req_query(req, "match-12");
  const char *match13 = papago_req_query(req, "match-13");
  const char *match14 = papago_req_query(req, "match-14");
  const char *match15 = papago_req_query(req, "match-15");
  const char *match16 = papago_req_query(req, "match-16");

  name = (name != NULL) ? name : "unknown";
  match1 = (match1 != NULL) ? match1 : "unknown";
  match2 = (match2 != NULL) ? match2 : "unknown";
  match3 = (match3 != NULL) ? match3 : "unknown";
  match4 = (match4 != NULL) ? match4 : "unknown";
  match5 = (match5 != NULL) ? match5 : "unknown";
  match6 = (match6 != NULL) ? match6 : "unknown";
  match7 = (match7 != NULL) ? match7 : "unknown";
  match8 = (match8 != NULL) ? match8 : "unknown";
  match9 = (match9 != NULL) ? match9 : "unknown";
  match10 = (match10 != NULL) ? match10 : "unknown";
  match11 = (match11 != NULL) ? match11 : "unknown";
  match12 = (match12 != NULL) ? match12 : "unknown";
  match13 = (match13 != NULL) ? match13 : "unknown";
  match14 = (match14 != NULL) ? match14 : "unknown";
  match15 = (match15 != NULL) ? match15 : "unknown";
  match16 = (match16 != NULL) ? match16 : "unknown";

  pthread_mutex_lock(&state->mutex);
  fprintf(state->file, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
          name, match1, match2, match3, match4, match5, match6, match7, match8,
          match9, match10, match11, match12, match13, match14, match15,
          match16);
  fflush(state->file);
  pthread_mutex_unlock(&state->mutex);

  papago_res_send(res, submit_html);
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // setup signal handling
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  shared_state_t state;
  pthread_mutex_init(&state.mutex, NULL);

  const char *csv_path = getenv("ANSWERS_CSV");
  if (csv_path == NULL) csv_path = "answers.csv";
  state.file = fopen(csv_path, "a");
  if (state.file == NULL) {
    fprintf(stderr, "failed to open %s\n", csv_path);
    return 1;
  }

  // create server
  server = papago_new();
  if (server == NULL) {
    fprintf(stderr, "failed to create server\n");
    return 1;
  }

  papago_middleware_t structured_logger = {
      .before = logger_before,
      .after = logger_after,
      .user_data = NULL,
  };
  papago_middleware_add(server, &structured_logger);

  // register HTTP routes
  papago_route(server, PAPAGO_GET, "/", index_handler, NULL);
  papago_route(server, PAPAGO_GET, "/submit", submit_handler, &state);

  papago_config_t config = papago_default_config();
  config.port = 8282;

  printf("Server starting on:\n");
  printf("  HTTP:      http://%s:%d\n", config.host, config.port);

  printf("Run\n\ncurl http://%s:%d/\n", config.host, config.port);
  printf("curl http://%s:%d/api/hello\n", config.host, config.port);
  printf("curl http://%s:%d/user/alice\n\n", config.host, config.port);

  // start server (blocking)
  if (papago_start(server, &config) != 0) {
    fprintf(stderr, "%s\n", papago_error());
    papago_destroy(server);

    return 1;
  }

  // cleanup
  papago_destroy(server);

  return 0;
}
