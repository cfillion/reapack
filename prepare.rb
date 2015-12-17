#!/usr/bin/env ruby

Signal.trap('INT') { abort }

REGEX = /\A\/\*(.+\n[^\n])+?\*\/\n/.freeze
PREAMBLE = DATA.read.freeze

def process(input)
  if input[0] == '/'
   output = input.sub REGEX, PREAMBLE
  else
   output = "%s\n%s" % [PREAMBLE, input]
  end
  
  input == output ? nil : output
end

ARGV.each do |path|
 File.open path, 'r+t' do |file|
   output = process file.read
   next unless output

   file.rewind
   file.write output
   file.truncate file.pos
 end
end

__END__
/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
