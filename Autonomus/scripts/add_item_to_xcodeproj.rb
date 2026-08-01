require 'xcodeproj'

FILE_TO_ADD = 'Autonomus/resources/click.cpp'
puts "STARTED RUBY SCRIPT"

Dir.glob('Autonomus/projects/*.xcodeproj').each do |proj_path|
  project = Xcodeproj::Project.open(proj_path)
  group = project.main_group.find_subpath('resources', true)

  already_present = group.files.any? { |f| f.path == 'click.cpp' }

  if already_present
    puts "click.cpp ya está en #{proj_path}, salteando."
  else
    file_ref = group.new_file(FILE_TO_ADD)
    project.targets.each do |target|
      target.source_build_phase.add_file_reference(file_ref)
    end
    project.save
    puts "Agregado click.cpp a #{proj_path}."
  end
end
