# mruby-yabm    [![build](https://github.com/yamori813/mruby-yabm/actions/workflows/ci.yml/badge.svg)](https://github.com/yamori813/mruby-yabm/actions/workflows/ci.yml)

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
t = YABM.new
t.print "Hello Bear Metal mruby"
```

## License
under the BSD License:
- see LICENSE file
