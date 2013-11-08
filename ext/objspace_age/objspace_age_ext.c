/**********************************************************************

  object_age_ext.c

  This file is covered under the Ruby's license.
  Original file is ruby/ext/objspace/object_tracing.c
  Some parts in this file are modified by Narihiro Nakamura.

**********************************************************************/

#include "ruby/ruby.h"
#include "ruby/debug.h"

size_t rb_gc_count(void); /* from gc.c */

struct traceobj_age_arg {
    VALUE newobj_trace;
    VALUE freeobj_trace;
    st_table *object_table;
    st_table *str_table;
};

/* TODO: do not use GLOBAL VARIABLE!!! */
struct traceobj_age_arg traceobj_age_arg;

struct allocation_info {
    const char *path;
    unsigned long line;
    size_t generation;
};

static const char *
make_unique_str(st_table *tbl, const char *str, long len)
{
    if (!str) {
	return NULL;
    }
    else {
	st_data_t n;
	char *result;

	if (st_lookup(tbl, (st_data_t)str, &n)) {
	    st_insert(tbl, (st_data_t)str, n+1);
	    st_get_key(tbl, (st_data_t)str, (st_data_t *)&result);
	}
	else {
	    result = (char *)ruby_xmalloc(len+1);
	    strncpy(result, str, len);
	    result[len] = 0;
	    st_add_direct(tbl, (st_data_t)result, 1);
	}
	return result;
    }
}

static void
delete_unique_str(st_table *tbl, const char *str)
{
    if (str) {
	st_data_t n;

	st_lookup(tbl, (st_data_t)str, &n);
	if (n == 1) {
	    st_delete(tbl, (st_data_t *)&str, 0);
	    ruby_xfree((char *)str);
	}
	else {
	    st_insert(tbl, (st_data_t)str, n-1);
	}
    }
}

static void
newobj_i(VALUE tpval, void *data)
{
    struct traceobj_age_arg *arg = (struct traceobj_age_arg *)data;
    rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
    VALUE obj = rb_tracearg_object(tparg);
    VALUE path = rb_tracearg_path(tparg);
    VALUE line = rb_tracearg_lineno(tparg);
    struct allocation_info *info = (struct allocation_info *)ruby_xmalloc(sizeof(struct allocation_info));
    const char *path_cstr = RTEST(path) ? make_unique_str(arg->str_table, RSTRING_PTR(path), RSTRING_LEN(path)) : 0;

    info->path = path_cstr;
    info->line = NUM2INT(line);
    info->generation = rb_gc_count();
    st_insert(arg->object_table, (st_data_t)obj, (st_data_t)info);
}

static void
freeobj_i(VALUE tpval, void *data)
{
    struct traceobj_age_arg *arg = (struct traceobj_age_arg *)data;
    rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
    VALUE obj = rb_tracearg_object(tparg);
    struct allocation_info *info;

    if (st_delete(arg->object_table, (st_data_t *)&obj, (st_data_t *)&info)) {
	delete_unique_str(arg->str_table, info->path);
	ruby_xfree(info);
    }
}

static int
free_keys_i(st_data_t key, st_data_t value, void *data)
{
    ruby_xfree((void *)key);
    return ST_CONTINUE;
}

static int
free_values_i(st_data_t key, st_data_t value, void *data)
{
    ruby_xfree((void *)value);
    return ST_CONTINUE;
}

static VALUE
stop_age_trace_object_allocations(VALUE objspace)
{
    struct traceobj_age_arg *arg = (struct traceobj_age_arg *)&traceobj_age_arg;
    rb_tracepoint_disable(arg->newobj_trace);
    rb_tracepoint_disable(arg->freeobj_trace);
    st_foreach(arg->object_table, free_values_i, 0);
    st_foreach(arg->str_table, free_keys_i, 0);
    st_free_table(arg->object_table);
    st_free_table(arg->str_table);

    arg->newobj_trace = (VALUE)NULL;

    return Qnil;
}

static VALUE
start_age_trace_object_allocations(VALUE objspace)
{
    traceobj_age_arg.newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, &traceobj_age_arg);
    traceobj_age_arg.freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, &traceobj_age_arg);
    traceobj_age_arg.object_table = st_init_numtable();
    traceobj_age_arg.str_table = st_init_strtable();

    rb_tracepoint_enable(traceobj_age_arg.newobj_trace);
    rb_tracepoint_enable(traceobj_age_arg.freeobj_trace);

    return Qnil;
}

struct allocation_info *
allocation_info(VALUE obj)
{
    if (traceobj_age_arg.newobj_trace) {
	struct allocation_info *info;
	if (st_lookup(traceobj_age_arg.object_table, obj, (st_data_t *)&info)) {
	    return info;
	}
    }
    return NULL;
}

static VALUE
allocation_sourceline(VALUE objspace, VALUE obj)
{
    struct allocation_info *info = allocation_info(obj);
    if (info) {
	return INT2FIX(info->line);
    }
    else {
	return Qnil;
    }
}

static VALUE
allocation_sourcefile_and_line(VALUE objspace, VALUE obj)
{
    struct allocation_info *info = allocation_info(obj);
    if (info) {
        if (info->path) {
            return rb_str_plus(rb_str_new2(info->path),
			       rb_str_plus(rb_str_new_cstr(":"),
					   rb_fix2str(allocation_sourceline(objspace, obj), 10)));
        }
	return Qnil;
    }
    else {
	return Qnil;
    }
}

static VALUE
allocation_old(VALUE objspace, VALUE obj)
{
    struct allocation_info *info = allocation_info(obj);
    if (info) {
	return SIZET2NUM(rb_gc_count() - info->generation);
    }
    else {
	return Qnil;
    }
}

struct arg_stat_data {
    VALUE result;
};

static int
age_stat_line_i(VALUE obj, struct allocation_info *info, st_data_t data)
{
    struct arg_stat_data *arg = (void*)data;
    long age = rb_gc_count() - info->generation;
    VALUE hash, cnt, path;

    hash = rb_ary_entry(arg->result, age);
    if (hash == Qnil) {
    	hash = rb_hash_new();
    	rb_ary_store(arg->result, age, hash);
    }
    path = allocation_sourcefile_and_line(Qnil, obj);
    cnt = rb_hash_aref(hash, path);
    if (cnt == Qnil) {
    	cnt = INT2NUM(0);
    }
    rb_hash_aset(hash, path, LONG2NUM(NUM2LONG(cnt)+1));
    return ST_CONTINUE;
}

static int
age_stat_class_i(VALUE obj, struct allocation_info *info, st_data_t data)
{
    struct arg_stat_data *arg = (void*)data;
    long age = rb_gc_count() - info->generation;
    VALUE hash, cnt, klass;

    hash = rb_ary_entry(arg->result, age);
    if (hash == Qnil) {
	hash = rb_hash_new();
	rb_ary_store(arg->result, age, hash);
    }
    klass = rb_obj_class(obj);
    cnt = rb_hash_aref(hash, klass);
    if (cnt == Qnil) {
	cnt = INT2NUM(0);
    }
    rb_hash_aset(hash, rb_obj_class(obj), LONG2NUM(NUM2LONG(cnt)+1));
    return ST_CONTINUE;
}

static VALUE
age_stats(VALUE objspace, VALUE type)
{
    struct arg_stat_data data;
    st_table *table = st_copy(traceobj_age_arg.object_table);
    VALUE result = rb_ary_new();

    data.result = result;
    if (type == ID2SYM(rb_intern("class"))) {
	st_foreach(table, age_stat_class_i, (st_data_t)&data);
    }
    else if (type == ID2SYM(rb_intern("line"))) {
	st_foreach(table, age_stat_line_i, (st_data_t)&data);
    }

    st_free_table(table);
    return result;
}

void
Init_objspace_age_ext(void)
{
    VALUE rb_mObjSpace = rb_const_get(rb_cObject, rb_intern("ObjectSpace"));
    VALUE rb_mObjSpaceAge = rb_const_get(rb_mObjSpace, rb_intern("Age"));

    rb_define_module_function(rb_mObjSpaceAge, "enable", start_age_trace_object_allocations, 0);
    rb_define_module_function(rb_mObjSpaceAge, "disable", stop_age_trace_object_allocations, 0);
    rb_define_module_function(rb_mObjSpaceAge, "birthplace", allocation_sourcefile_and_line, 1);
    rb_define_module_function(rb_mObjSpaceAge, "how_old", allocation_old, 1);
    rb_define_module_function(rb_mObjSpaceAge, "stats_on", age_stats, 1);
}
