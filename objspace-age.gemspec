# -*- encoding: utf-8 -*-
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'objspace/age/version'

Gem::Specification.new do |gem|
  gem.name          = "objspace-age"
  gem.version       = ObjectSpace::Age::VERSION
  gem.authors       = ["Narihiro Nakamura"]
  gem.email         = ["authornari@gmail.com"]
  gem.description   = %q{Show stats on object's age}
  gem.summary       = %q{Show stats on object's age}
  gem.homepage      = "https://github.com/authorNari/objspace-age"

  gem.files         = `git ls-files`.split($/)
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.require_paths = ["lib", "ext"]
end
