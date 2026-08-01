require 'xcodeproj'

FILE_TO_ADD = 'click.cpp'
puts "STARTED RUBY SCRIPT"


proj_path = "projects/Autonomus-macOS.xcodeproj"

project = Xcodeproj::Project.open(proj_path)
group = project.main_group.find_subpath('sources', true)

already_present = group.files.any? { |f| f.path == 'click.cpp' }

if already_present
    puts "click.cpp ya está en #{proj_path}, salteando."
else
    file_ref = group.new_file(FILE_TO_ADD)
    project.targets.each do |target|
        next unless target.respond_to?(:source_build_phase)
        target.source_build_phase.add_file_reference(file_ref)
    end
project.save
puts "Agregado click.cpp a #{proj_path}."
end
