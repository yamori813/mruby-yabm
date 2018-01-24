MRuby::Build.new do |conf|
  toolchain :gcc
  conf.gembox 'default'
  conf.gem '../mruby-rtlbm-rtl8196c'
  conf.enable_test
end
