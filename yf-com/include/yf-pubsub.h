/*
 * YF
 * yf-pubsub.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_PUBSUB_H
#define YF_YF_PUBSUB_H

#include "yf-defs.h"

YF_DECLS_BEGIN

/**
 * Publish-Subscribe values.
 */
#define YF_PUBSUB_NONE   0
#define YF_PUBSUB_DEINIT 0x01
#define YF_PUBSUB_CHANGE 0x02

/**
 * Sets a publisher object.
 *
 * @param pub: The publisher.
 * @param pubsub_mask: The 'YF_PUBSUB' mask indicating what can be published.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_setpub(const void *pub, unsigned pubsub_mask);

/**
 * Checks an object's publish-subscribe mask.
 *
 * @param pub: The publisher.
 * @return: The 'YF_PUBSUB' mask of 'pub'.
 */
unsigned yf_checkpub(const void *pub);

/**
 * Publishes.
 *
 * @param pub: The publisher.
 * @param pubsub: The 'YF_PUBSUB' value indicating what to publish.
 */
void yf_publish(const void *pub, int pubsub);

/**
 * Subscribes.
 *
 * @param pub: The publisher.
 * @param sub: The subscriber.
 * @param pubsub_mask: The 'YF_PUBSUB' mask indicating what to subscribe to.
 * @param callb: The callback to execute when publishing takes place.
 * @param arg: The generic argument to pass on 'callb' calls. Can be 'NULL'.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_subscribe(const void *pub, const void *sub, unsigned pubsub_mask,
    void (*callb)(void *pub, int pubsub, void *arg), void *arg);

YF_DECLS_END

#endif /* YF_YF_PUBSUB_H */
