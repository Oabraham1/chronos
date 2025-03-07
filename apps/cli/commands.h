/**
 * @file commands.h
 * @brief Command handlers for CLI application
 */

#ifndef CHRONOS_COMMANDS_H
#define CHRONOS_COMMANDS_H

#include "chronos.h"

namespace chronos
{
    namespace cli
    {

        /**
         * @brief Execute the 'create' command
         *
         * @param partitioner Reference to partitioner object
         * @param argc Argument count
         * @param argv Argument values
         * @return Exit code
         */
        int executeCreate(ChronosPartitioner &partitioner, int argc, char *argv[]);

        /**
         * @brief Execute the 'list' command
         *
         * @param partitioner Reference to partitioner object
         * @return Exit code
         */
        int executeList(ChronosPartitioner &partitioner);

        /**
         * @brief Execute the 'release' command
         *
         * @param partitioner Reference to partitioner object
         * @param argc Argument count
         * @param argv Argument values
         * @return Exit code
         */
        int executeRelease(ChronosPartitioner &partitioner, int argc, char *argv[]);

        /**
         * @brief Execute the 'stats' command
         *
         * @param partitioner Reference to partitioner object
         * @return Exit code
         */
        int executeStats(ChronosPartitioner &partitioner);

        /**
         * @brief Execute the 'available' command
         *
         * @param partitioner Reference to partitioner object
         * @param argc Argument count
         * @param argv Argument values
         * @return Exit code
         */
        int executeAvailable(ChronosPartitioner &partitioner, int argc, char *argv[]);

        /**
         * @brief Execute the 'help' command
         *
         * @return Exit code
         */
        int executeHelp();

    }
}

#endif
