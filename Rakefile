task :default => [:prepare, :build]

task :prepare do
  sources = FileList['src/*.{h,c}pp']
  ruby 'prepare.rb', *sources, :verbose => false
end

task :build, [:variants] do |_, args|
  vars = Array(args[:variants]) + args.extras
  vars.reject &:empty?

  sh 'tup', *vars, :verbose => false
end

task :test do
  FileList['x{86,64}/bin/test{,.exe}'].each {|exe|
    sh exe
  }
end
