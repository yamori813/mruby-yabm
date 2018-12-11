# mruby-yabm   [![Build Status](https://travis-ci.org/yamori813/mruby-yabm.svg?branch=master)](https://travis-ci.org/yamori813/mruby-yabm)
Yet Another Bare Metal class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'yamori813/mruby-yabm'
end
```
## example
```ruby
t = YABM.new(YABM::RTL8196C_GENERIC)
t.print "Hello Bear Metal mruby"
```

## License
under the BSD License:
- see LICENSE file
