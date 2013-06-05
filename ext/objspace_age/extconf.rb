require 'mkmf'

if with_config("test")
  $defs << "-DOBJSPACE_HISTGRAM_TEST"
  $CFLAGS << " -O0"
end

dir_config("objspace_age_ext")
create_header
create_makefile("objspace_age_ext")
