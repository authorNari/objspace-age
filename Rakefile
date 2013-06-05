require "bundler/gem_tasks"

task :compile do
  sh "cd ext/objspace_age/ && ruby extconf.rb && make"
end

task :clean do
  sh "cd ext/objspace_age/ && rm -f *.o *.so"
end

task :test => [:clean, :compile] do
  sh "/usr/bin/env ruby test/run-test.rb"
end
