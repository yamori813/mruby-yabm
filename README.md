# mruby-rtlbm-rtl8196c   [![Build Status](https://travis-ci.org/yamori813/mruby-rtlbm-rtl8196c.svg?branch=master)](https://travis-ci.org/yamori813/mruby-rtlbm-rtl8196c)
RTL8196C class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'yamori813/mruby-rtlbm-rtl8196c'
end
```
## example
```ruby
p RTL8196C.hi
#=> "hi!!"
t = RTL8196C.new "hello"
p t.hello
#=> "hello"
p t.bye
#=> "hello bye"
```

## License
under the BSD License:
- see LICENSE file
