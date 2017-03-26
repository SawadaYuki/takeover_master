#!/usr/bin/ruby
# -*- coding: utf-8 -*-

path=Dir::pwd
#puts path


Dir::glob("#{path}/ckpt*").each {|f|
 if File::ftype(f) == "directory"
    #puts "this is directory =#{f}"
    ## サブディレクトリを階層が深い順にソートした配列を作成
    dirlist=Dir::glob("#{f}/"+"**/").sort{
    #split('/')で/を区切り文字として文字列として分割し、配列に返す
     |a,b| b.split('/').size <=> a.split('/').size
    }
    puts "dirlist=#{dirlist}"
    #指定したディレクトリ内にある全てのファイル(とディレクトリ)に対して処理を行える
    ## サブディレクトリ配下の全ファイルを削除後、サブディレクトリを削除
    dirlist.each { |d|
       Dir::foreach(d) { |subfile|
            File::delete(d+subfile) if ! (/\.+$/=~subfile)
       }
      Dir::rmdir(d)
    }
 else
  f = File.basename(f)
  puts "#{f}:#{File::stat(f).size} bytes"
  File.delete(f)
  jug=File.exist?(f)
  if jug = "true"
     #puts "successful of delete"
  end
 end

}


Dir::glob("#{path}/dmtcp*").each {|f|
 if File::ftype(f) == "directory"
    #puts "this is directory =#{f}"
    dirlist=Dir::glob("#{f}/"+"**/").sort{
     |a,b| b.split('/').size <=> a.split('/').size
    }
    dirlist.each { |d|
       Dir::foreach(d) { |subfile|
            File::delete(d+subfile) if ! (/\.+$/=~subfile)
       }
      Dir::rmdir(d)
    }
 else
  #puts "#{f}:#{File::stat(f).size} bytes"
  File.delete(f)
  jug=File.exist?(f)
  if jug = "true"
     #puts "successful of delete"
  end
 end
}











