/*
 * Copyright 2017 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <php.h>
#include <Zend/zend_interfaces.h>

#include "phongo_compat.h"
#include "php_phongo.h"
#include "php_bson.h"

zend_class_entry *php_phongo_session_ce;

static bool php_phongo_session_get_timestamp_parts(zval *obj, uint32_t *timestamp, uint32_t *increment TSRMLS_DC)
{
	bool retval = false;
#if PHP_VERSION_ID >= 70000
	zval ztimestamp;
	zval zincrement;

	zend_call_method_with_0_params(obj, NULL, NULL, "getTimestamp", &ztimestamp);

	if (Z_ISUNDEF(ztimestamp) || EG(exception)) {
		goto cleanup;
	}

	zend_call_method_with_0_params(obj, NULL, NULL, "getIncrement", &zincrement);

	if (Z_ISUNDEF(zincrement) || EG(exception)) {
		goto cleanup;
	}

	*timestamp = Z_LVAL(ztimestamp);
	*increment = Z_LVAL(zincrement);
#else
	zval *ztimestamp = NULL;
	zval *zincrement = NULL;

	zend_call_method_with_0_params(&obj, NULL, NULL, "getTimestamp", &ztimestamp);

	if (Z_ISUNDEF(ztimestamp) || EG(exception)) {
		goto cleanup;
	}

	zend_call_method_with_0_params(&obj, NULL, NULL, "getIncrement", &zincrement);

	if (Z_ISUNDEF(zincrement) || EG(exception)) {
		goto cleanup;
	}

	*timestamp = Z_LVAL_P(ztimestamp);
	*increment = Z_LVAL_P(zincrement);
#endif

	retval = true;

cleanup:
	if (!Z_ISUNDEF(ztimestamp)) {
		zval_ptr_dtor(&ztimestamp);
	}

	if (!Z_ISUNDEF(zincrement)) {
		zval_ptr_dtor(&zincrement);
	}

	return retval;
}

/* {{{ proto void MongoDB\Driver\Session::advanceClusterTime(array|object $clusterTime)
   Advances the cluster time for this Session */
static PHP_METHOD(Session, advanceClusterTime)
{
	php_phongo_session_t *intern;
	zval                 *zcluster_time;
	bson_t                cluster_time = BSON_INITIALIZER;
	SUPPRESS_UNUSED_WARNING(return_value_ptr) SUPPRESS_UNUSED_WARNING(return_value_used)


	intern = Z_SESSION_OBJ_P(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "A", &zcluster_time) == FAILURE) {
		return;
	}

	php_phongo_zval_to_bson(zcluster_time, PHONGO_BSON_NONE, &cluster_time, NULL TSRMLS_CC);

	/* An exception may be thrown during BSON conversion */
	if (EG(exception)) {
		goto cleanup;
	}

	mongoc_client_session_advance_cluster_time(intern->client_session, &cluster_time);

cleanup:
	bson_destroy(&cluster_time);
} /* }}} */

/* {{{ proto void MongoDB\Driver\Session::advanceOperationTime(MongoDB\BSON\TimestampInterface $timestamp)
   Advances the operation time for this Session */
static PHP_METHOD(Session, advanceOperationTime)
{
	php_phongo_session_t *intern;
	zval                 *ztimestamp;
	uint32_t              timestamp = 0;
	uint32_t              increment = 0;
	SUPPRESS_UNUSED_WARNING(return_value_ptr) SUPPRESS_UNUSED_WARNING(return_value_used)


	intern = Z_SESSION_OBJ_P(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &ztimestamp, php_phongo_timestamp_interface_ce) == FAILURE) {
		return;
	}

	if (!php_phongo_session_get_timestamp_parts(ztimestamp, &timestamp, &increment TSRMLS_CC)) {
		return;
	}

	mongoc_client_session_advance_operation_time(intern->client_session, timestamp, increment);
} /* }}} */

/* {{{ proto object|null MongoDB\Driver\Session::getClusterTime()
   Returns the cluster time for this Session */
static PHP_METHOD(Session, getClusterTime)
{
	php_phongo_session_t  *intern;
	const bson_t          *cluster_time;
	php_phongo_bson_state  state = PHONGO_BSON_STATE_INITIALIZER;
	SUPPRESS_UNUSED_WARNING(return_value_ptr) SUPPRESS_UNUSED_WARNING(return_value_used)


	intern = Z_SESSION_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	cluster_time = mongoc_client_session_get_cluster_time(intern->client_session);

	if (!cluster_time) {
		RETURN_NULL();
	}

	if (!php_phongo_bson_to_zval_ex(bson_get_data(cluster_time), cluster_time->len, &state)) {
		/* Exception should already have been thrown */
		zval_ptr_dtor(&state.zchild);
		return;
	}

#if PHP_VERSION_ID >= 70000
	RETURN_ZVAL(&state.zchild, 0, 1);
#else
	RETURN_ZVAL(state.zchild, 0, 1);
#endif
} /* }}} */

/* {{{ proto object MongoDB\Driver\Session::getLogicalSessionId()
   Returns the logical session ID for this Session */
static PHP_METHOD(Session, getLogicalSessionId)
{
	php_phongo_session_t  *intern;
	const bson_t          *lsid;
	php_phongo_bson_state  state = PHONGO_BSON_STATE_INITIALIZER;
	SUPPRESS_UNUSED_WARNING(return_value_ptr) SUPPRESS_UNUSED_WARNING(return_value_used)


	intern = Z_SESSION_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	lsid = mongoc_client_session_get_lsid(intern->client_session);


	if (!php_phongo_bson_to_zval_ex(bson_get_data(lsid), lsid->len, &state)) {
		/* Exception should already have been thrown */
		zval_ptr_dtor(&state.zchild);
		return;
	}

#if PHP_VERSION_ID >= 70000
	RETURN_ZVAL(&state.zchild, 0, 1);
#else
	RETURN_ZVAL(state.zchild, 0, 1);
#endif
} /* }}} */

/* {{{ proto MongoDB\BSON\Timestamp|null MongoDB\Driver\Session::getOperationTime()
   Returns the operation time for this Session */
static PHP_METHOD(Session, getOperationTime)
{
	php_phongo_session_t *intern;
	uint32_t              timestamp, increment;
	SUPPRESS_UNUSED_WARNING(return_value_ptr) SUPPRESS_UNUSED_WARNING(return_value_used)


	intern = Z_SESSION_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	mongoc_client_session_get_operation_time(intern->client_session, &timestamp, &increment);

	/* mongoc_client_session_get_operation_time() returns 0 for both parts if
	 * the session has not been used. According to the causal consistency spec,
	 * the operation time for an unused session is null. */
	if (timestamp == 0 && increment == 0) {
		RETURN_NULL();
	}

	php_phongo_new_timestamp_from_increment_and_timestamp(return_value, increment, timestamp TSRMLS_CC);
} /* }}} */

/* {{{ MongoDB\Driver\Session function entries */
ZEND_BEGIN_ARG_INFO_EX(ai_Session_advanceClusterTime, 0, 0, 1)
	ZEND_ARG_INFO(0, clusterTime)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Session_advanceOperationTime, 0, 0, 1)
	ZEND_ARG_INFO(0, timestamp)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Session_void, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_phongo_session_me[] = {
	PHP_ME(Session, advanceClusterTime, ai_Session_advanceClusterTime, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Session, advanceOperationTime, ai_Session_advanceOperationTime, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Session, getClusterTime, ai_Session_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Session, getLogicalSessionId, ai_Session_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Session, getOperationTime, ai_Session_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_NAMED_ME(__construct, PHP_FN(MongoDB_disabled___construct), ai_Session_void, ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
	ZEND_NAMED_ME(__wakeup, PHP_FN(MongoDB_disabled___wakeup), ai_Session_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_FE_END
};
/* }}} */

/* {{{ MongoDB\Driver\Session object handlers */
static zend_object_handlers php_phongo_handler_session;

static void php_phongo_session_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_session_t *intern = Z_OBJ_SESSION(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	if (intern->client_session) {
		mongoc_client_session_destroy(intern->client_session);
	}

#if PHP_VERSION_ID < 70000
	efree(intern);
#endif
} /* }}} */

static phongo_create_object_retval php_phongo_session_create_object(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	php_phongo_session_t *intern = NULL;

	intern = PHONGO_ALLOC_OBJECT_T(php_phongo_session_t, class_type);

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

#if PHP_VERSION_ID >= 70000
	intern->std.handlers = &php_phongo_handler_session;

	return &intern->std;
#else
	{
		zend_object_value retval;
		retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_phongo_session_free_object, NULL TSRMLS_CC);
		retval.handlers = &php_phongo_handler_session;

		return retval;
	}
#endif
} /* }}} */

static HashTable *php_phongo_session_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	php_phongo_session_t       *intern = NULL;
	const mongoc_session_opt_t *cs_opts;
#if PHP_VERSION_ID >= 70000
	zval                        retval;
#else
	zval                        retval = zval_used_for_init;
#endif

	*is_temp = 1;
	intern = Z_SESSION_OBJ_P(object);

	array_init(&retval);

	{
		const bson_t *lsid;

		php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;
		/* Use native arrays for debugging output */
		state.map.root_type = PHONGO_TYPEMAP_NATIVE_ARRAY;
		state.map.document_type = PHONGO_TYPEMAP_NATIVE_ARRAY;

		lsid = mongoc_client_session_get_lsid(intern->client_session);

		php_phongo_bson_to_zval_ex(bson_get_data(lsid), lsid->len, &state);

#if PHP_VERSION_ID >= 70000
		ADD_ASSOC_ZVAL_EX(&retval, "logicalSessionId", &state.zchild);
#else
		ADD_ASSOC_ZVAL_EX(&retval, "logicalSessionId", state.zchild);
#endif
	}

	{
		const bson_t *cluster_time;

		php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;
		/* Use native arrays for debugging output */
		state.map.root_type = PHONGO_TYPEMAP_NATIVE_ARRAY;
		state.map.document_type = PHONGO_TYPEMAP_NATIVE_ARRAY;

		cluster_time = mongoc_client_session_get_cluster_time(intern->client_session);

		if (cluster_time) {
			php_phongo_bson_to_zval_ex(bson_get_data(cluster_time), cluster_time->len, &state);

#if PHP_VERSION_ID >= 70000
			ADD_ASSOC_ZVAL_EX(&retval, "clusterTime", &state.zchild);
#else
			ADD_ASSOC_ZVAL_EX(&retval, "clusterTime", state.zchild);
#endif
		} else {
			ADD_ASSOC_NULL_EX(&retval, "clusterTime");
		}
	}

	cs_opts = mongoc_client_session_get_opts(intern->client_session);
	ADD_ASSOC_BOOL_EX(&retval, "causalConsistency", mongoc_session_opts_get_causal_consistency(cs_opts));

	{
		uint32_t timestamp, increment;

		mongoc_client_session_get_operation_time(intern->client_session, &timestamp, &increment);

		if (timestamp && increment) {
#if PHP_VERSION_ID >= 70000
			zval ztimestamp;

			php_phongo_new_timestamp_from_increment_and_timestamp(&ztimestamp, increment, timestamp TSRMLS_CC);
			ADD_ASSOC_ZVAL_EX(&retval, "operationTime", &ztimestamp);
#else
			zval *ztimestamp;

			MAKE_STD_ZVAL(ztimestamp);
			php_phongo_new_timestamp_from_increment_and_timestamp(ztimestamp, increment, timestamp TSRMLS_CC);
			ADD_ASSOC_ZVAL_EX(&retval, "operationTime", ztimestamp);
#endif
		} else {
			ADD_ASSOC_NULL_EX(&retval, "operationTime");
		}
	}

	return Z_ARRVAL(retval);
} /* }}} */
/* }}} */

void php_phongo_session_init_ce(INIT_FUNC_ARGS) /* {{{ */
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "MongoDB\\Driver", "Session", php_phongo_session_me);
	php_phongo_session_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_session_ce->create_object = php_phongo_session_create_object;
	PHONGO_CE_FINAL(php_phongo_session_ce);
	PHONGO_CE_DISABLE_SERIALIZATION(php_phongo_session_ce);

	memcpy(&php_phongo_handler_session, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_phongo_handler_session.get_debug_info = php_phongo_session_get_debug_info;
#if PHP_VERSION_ID >= 70000
	php_phongo_handler_session.free_obj = php_phongo_session_free_object;
	php_phongo_handler_session.offset = XtOffsetOf(php_phongo_session_t, std);
#endif
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
