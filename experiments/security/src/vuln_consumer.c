#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#include "cJSON.h"

#define TZ_BUF_SIZE 8

static sigjmp_buf            g_recover;
static volatile sig_atomic_t g_caught = 0;

static void fault_handler(int sig) {
    g_caught = sig;
    siglongjmp(g_recover, 1);
}

static void install_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fault_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGSEGV, &sa, NULL);
#ifdef SIGPROT
    sigaction(SIGPROT, &sa, NULL);
#endif
}

static const char *signal_label(int sig) {
#ifdef SIGPROT
    if (sig == SIGPROT) return "SIGPROT (violacao de capability CHERI)";
#endif
    if (sig == SIGSEGV) return "SIGSEGV (falha de segmentacao)";
    return "sinal desconhecido";
}

static void consume_timezone(const char *tz) {
    char *dst = (char *)malloc(TZ_BUF_SIZE);
    if (!dst) { fprintf(stderr, "  [sink] malloc falhou\n"); return; }

    printf("  [sink] buffer de destino: %d bytes (heap)\n", TZ_BUF_SIZE);
    printf("  [sink] copiando %zu bytes via strcpy()...\n", strlen(tz) + 1);

    strcpy(dst, tz);

    /* Só se chega aqui se NÃO houve violação (entrada benigna, ou baseline
     * que aceitou silenciosamente o overflow). */
    printf("  [sink] strcpy concluido. Conteudo (%zu bytes): \"%s\"\n",
           strlen(dst), dst);
    free(dst);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "uso: %s <payload.json>\n", argv[0]);
        return 2;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 2; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char *raw = (char *)malloc((size_t)n + 1);
    if (!raw || fread(raw, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "falha ao ler %s\n", argv[1]);
        fclose(f); free(raw); return 2;
    }
    raw[n] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(raw);
    if (!root) { fprintf(stderr, "JSON invalido\n"); free(raw); return 2; }

    cJSON *meta = cJSON_GetObjectItem(root, "meta");
    cJSON *tz   = meta ? cJSON_GetObjectItem(meta, "timezone") : NULL;
    if (!cJSON_IsString(tz) || !tz->valuestring) {
        fprintf(stderr, "campo meta.timezone ausente ou invalido\n");
        cJSON_Delete(root); free(raw); return 2;
    }

    printf("[consumidor] campo timezone recebido: %zu caracteres\n",
           strlen(tz->valuestring));

    install_handlers();

    if (sigsetjmp(g_recover, 1) == 0) {
        consume_timezone(tz->valuestring);
        printf("[resultado] processamento concluido SEM violacao detectada.\n");
        if (strlen(tz->valuestring) >= TZ_BUF_SIZE) {
            printf("[resultado] ATENCAO: o overflow foi aceito silenciosamente "
                   "(memoria corrompida).\n");
        }
        cJSON_Delete(root);
        free(raw);
        return 0;
    } else {
        printf("[resultado] *** %s ***\n", signal_label(g_caught));
        printf("[resultado] escrita fora dos limites INTERCEPTADA no ponto da "
               "violacao; corrupcao de memoria evitada.\n");
        return 1;
    }
}
