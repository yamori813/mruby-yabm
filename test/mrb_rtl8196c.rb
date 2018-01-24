##
## RTL8196C Test
##

assert("RTL8196C#hello") do
  t = RTL8196C.new "hello"
  assert_equal("hello", t.hello)
end

assert("RTL8196C#bye") do
  t = RTL8196C.new "hello"
  assert_equal("hello bye", t.bye)
end

assert("RTL8196C.hi") do
  assert_equal("hi!!", RTL8196C.hi)
end
