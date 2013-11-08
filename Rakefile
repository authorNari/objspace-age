require "bundler/gem_tasks"

desc "compile"
task :compile do
  sh "cd ext/objspace_age/ && ruby extconf.rb && make"
end

desc "clean"
task :clean do
  sh "cd ext/objspace_age/ && rm -f *.o *.so"
end

desc "test"
task :test => [:clean, :compile] do
  sh "/usr/bin/env ruby test/run-test.rb"
end
