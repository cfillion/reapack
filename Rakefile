SOURCES = FileList['src/*.{h,c}pp']

task :default => [:prepare, :build]

task :prepare do
  ruby 'prepare.rb', *SOURCES, :verbose => false
end

task :build do
  sh 'tup', :verbose => false
end

task :test do
  FileList['x{86,64}/bin/test{,.exe}'].each {|exe|
    sh exe
  }
end
