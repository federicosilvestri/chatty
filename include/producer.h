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
 * Wait termination of thread.
 */
void producer_wait();

/**
 * Destroy the producer
 */
void producer_destroy();
