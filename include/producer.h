#include <stdbool.h>
#include <libconfig.h>

extern struct config_t server_conf;

/**
 * @brief initializes all workspace for producer
 * @return true on success, false on error
 */
bool producer_init();

/**
 * Start the producer thread.
 *
 * @return true on success, false on error
 */
bool producer_start();

/**
 * Destroy producer
 */
void producer_destroy();
