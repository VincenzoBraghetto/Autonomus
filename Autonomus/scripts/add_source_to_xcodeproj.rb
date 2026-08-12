require 'xcodeproj'

files_to_add = ['../click.cpp', '../click.hpp']
PROJ_PATH = "Autonomus-macOS.xcodeproj"

project = Xcodeproj::Project.open(PROJ_PATH)
group = project.main_group.find_subpath('sources', true)

files_to_add.each do |file|
    existing = group.files.select { |f| f.path == file }
    existing.each do |file_ref|
        puts "Removing existing '#{file}' from '#{PROJ_PATH}'"
        file_ref.remove_from_project
    end
end

files_to_add.each do |file|
    already_present = group.files.any? { |f| f.path == file }

    if already_present
        puts "'#{file}' already in '#{PROJ_PATH}', skipping."
        next
    end

    puts "Adding '#{file}' to xcode project '#{PROJ_PATH}'"
    file_ref = group.new_file(file)

    project.targets.each do |target|
        next unless target.respond_to?(:source_build_phase)
        target.source_build_phase.add_file_reference(file_ref)
    end

    puts "Added '#{file}' to '#{PROJ_PATH}'."
end

project.save
puts "Done adding files to xcode project."
