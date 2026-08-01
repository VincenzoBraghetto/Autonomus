require 'xcodeproj'

FILE_TO_ADD = '../click.cpp'
PROJ_PATH = "projects/Autonomus-macOS.xcodeproj"

puts "Adding #{FILE_TO_ADD} to xcode project #{PROJ_PATH}"

project = Xcodeproj::Project.open(PROJ_PATH)
group = project.main_group.find_subpath('sources', true)

already_present = group.files.any? { |f| f.path == FILE_TO_ADD }

if already_present
    puts "#{FILE_TO_ADD} already in #{PROJ_PATH}, skipping."
else
    file_ref = group.new_file(FILE_TO_ADD)
    project.targets.each do |target|
        next unless target.respond_to?(:source_build_phase)
        target.source_build_phase.add_file_reference(file_ref)
    end
project.save
puts "Added #{FILE_TO_ADD} to #{PROJ_PATH}."
end
