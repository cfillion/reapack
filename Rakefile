task :default => [:prepare, :build, :test]

desc 'Prepare the source code for building using prepare.rb'
task :prepare do
  sources = FileList['src/*.{{h,c}pp,mm}']
  ruby 'prepare.rb', *sources, :verbose => false
end

desc 'Build ReaPack (by default both 64-bit and 32-bit builds are generated)'
task :build, [:variants] do |_, args|
  vars = Array(args[:variants]) + args.extras
  vars.reject &:empty?

  if Gem.win_platform? && ENV['VCINSTALLDIR'].nil?
    raise "VCINSTALLDIR is unset. Is this Developer Command Prompt for Visual Studio?"
  end

  sh 'tup', *vars, :verbose => false
end

desc 'Run the test suite for all architectures'
task :test do
  FileList['x{86,64}/bin/test{,.exe}'].each {|exe|
    sh exe
  }
end
