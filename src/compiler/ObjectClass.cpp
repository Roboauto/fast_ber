#include "fast_ber/compiler/ObjectClass.hpp"
#include "fast_ber/compiler/ResolveType.hpp"

void object_class_to_concrete(Asn1Tree& tree, Module& module, Type& type);

void object_class_to_concrete(Asn1Tree& tree, Module& module, ChoiceType& choice)
{
    for (NamedType& named_type : choice.choices)
    {
        object_class_to_concrete(tree, module, named_type.type);
    }
}
void object_class_to_concrete(Asn1Tree& tree, Module& module, SequenceType& sequence)
{
    for (ComponentType& component : sequence.components)
    {
        object_class_to_concrete(tree, module, component.named_type.type);
    }
}
void object_class_to_concrete(Asn1Tree& tree, Module& module, SequenceOfType& sequence)
{
    Type& type = sequence.has_name ? sequence.named_type->type : *sequence.type;
    object_class_to_concrete(tree, module, type);
}
void object_class_to_concrete(Asn1Tree& tree, Module& module, SetType& set)
{
    for (ComponentType& component : set.components)
    {
        object_class_to_concrete(tree, module, component.named_type.type);
    }
}
void object_class_to_concrete(Asn1Tree& tree, Module& module, SetOfType& set)
{
    Type& type = set.has_name ? set.named_type->type : *set.type;
    object_class_to_concrete(tree, module, type);
}
void object_class_to_concrete(Asn1Tree& tree, Module& module, PrefixedType& prefixed_type)
{
    object_class_to_concrete(tree, module, prefixed_type.tagged_type->type);
}

void object_class_to_concrete(Asn1Tree& tree, Module& module, BuiltinType& type)
{
    if (std::holds_alternative<ChoiceType>(type))
    {
        object_class_to_concrete(tree, module, std::get<ChoiceType>(type));
    }
    else if (std::holds_alternative<SequenceType>(type))
    {
        object_class_to_concrete(tree, module, std::get<SequenceType>(type));
    }
    else if (std::holds_alternative<SequenceOfType>(type))
    {
        object_class_to_concrete(tree, module, std::get<SequenceOfType>(type));
    }
    else if (std::holds_alternative<SetType>(type))
    {
        object_class_to_concrete(tree, module, std::get<SetType>(type));
    }
    else if (std::holds_alternative<SetOfType>(type))
    {
        object_class_to_concrete(tree, module, std::get<SetOfType>(type));
    }
    else if (std::holds_alternative<PrefixedType>(type))
    {
        object_class_to_concrete(tree, module, std::get<PrefixedType>(type));
    }
}

Type create_concrete_type(Asn1Tree& tree, Module& module, ObjectClassFieldType& object_class_field)
{
    const Assignment& assigment = resolve(tree, module.module_reference, object_class_field.referenced_object_class);
    if (!is_object_class(assigment))
    {
        throw std::runtime_error("Referenced object is not an ObjectClass " +
                                 object_class_field.referenced_object_class.type_reference);
    }

    if (object_class_field.fieldnames.size() == 1)
    {
        for (const ClassField& field : object_class(assigment).fields)
        {
            if (field.name == object_class_field.fieldnames[0])
            {
                if (std::holds_alternative<FixedTypeValueField>(field.field))
                {
                    return std::get<FixedTypeValueField>(field.field).type;
                }
                throw std::runtime_error("Referenced class field does not have a type: " +
                                         object_class_field.referenced_object_class.type_reference + "." +
                                         object_class_field.fieldnames[0]);
            }
        }
    }

    throw std::runtime_error("Failed to parse object field reference " +
                             object_class_field.referenced_object_class.type_reference);
}

void object_class_to_concrete(Asn1Tree& tree, Module& module, Type& type)
{
    if (std::holds_alternative<BuiltinType>(type))
    {
        if (std::holds_alternative<ObjectClassFieldType>(std::get<BuiltinType>(type)))
        {
            type = create_concrete_type(tree, module, std::get<ObjectClassFieldType>(std::get<BuiltinType>(type)));
        }
        else
        {
            object_class_to_concrete(tree, module, std::get<BuiltinType>(type));
        }
    }
    else if (std::holds_alternative<DefinedType>(type))
    {
        // Do nothing
    }
    else
    {
        throw std::runtime_error("Unhandled type!");
    }
}

bool is_defined_object_class(const std::string& module_reference, const std::string& type_reference,
                             const std::unordered_set<std::string>& object_class_names)
{
    return object_class_names.count(module_reference + "." + type_reference) > 0;
}

bool is_defined_object_class(const Asn1Tree&, Module& module, const Type& type,
                             const std::unordered_set<std::string>& object_class_names)
{
    if (is_defined(type))
    {
        const DefinedType& defined = std::get<DefinedType>(type);
        if (defined.module_reference)
        {
            return is_defined_object_class(*defined.module_reference, defined.type_reference, object_class_names);
        }
        return is_defined_object_class(module.module_reference, defined.type_reference, object_class_names);
    }
    return false;
}

void remove_object_classes(Asn1Tree& tree, const std::unordered_set<std::string>& object_class_names)
{
    for (Module& module : tree.modules)
    {
        module.assignments.erase(
            std::remove_if(
                module.assignments.begin(), module.assignments.end(),
                [&](const Assignment& assignment) {
                    if (is_object_class(assignment))
                    {
                        return true;
                    }
                    else if (is_type(assignment) && is_defined(type(assignment)))
                    {
                        return is_defined_object_class(tree, module, type(assignment), object_class_names);
                    }
                    else if (std::holds_alternative<ValueAssignment>(assignment.specific) &&
                             std::holds_alternative<DefinedType>(std::get<ValueAssignment>(assignment.specific).type))
                    {
                        const ValueAssignment& value_assign = std::get<ValueAssignment>(assignment.specific);
                        return is_defined_object_class(tree, module, value_assign.type, object_class_names);
                    }

                    for (const Parameter& parameter : assignment.parameters)
                    {
                        if (parameter.governor)
                        {
                            if (is_defined_object_class(tree, module, *parameter.governor, object_class_names))
                            {
                                return true;
                            }
                        }
                    }

                    return false;
                }),
            module.assignments.end());
    }

    for (Module& module : tree.modules)
    {
        for (Import& import : module.imports)
        {
            import.imported_types.erase(std::remove_if(import.imported_types.begin(), import.imported_types.end(),
                                                       [&](const std::string& imported_name) {
                                                           return is_defined_object_class(import.module_reference,
                                                                                          imported_name,
                                                                                          object_class_names);
                                                       }),
                                        import.imported_types.end());
        }
    }
}

std::unordered_set<std::string> get_object_class_names(const Asn1Tree& tree)
{
    std::unordered_set<std::string> object_class_names;

    size_t old_number_of_names = 0;
    do
    {
        old_number_of_names = object_class_names.size();
        for (const Module& module : tree.modules)
        {
            for (const Assignment& assignment : module.assignments)
            {
                if (is_type(assignment) && is_defined(type(assignment)))
                {
                    const DefinedType& defined = std::get<DefinedType>(type(assignment));
                    if (!is_a_parameter(defined.type_reference, assignment.parameters))
                    {
                        const Assignment& inner_assignment = resolve(tree, module.module_reference, defined);
                        if (is_object_class(inner_assignment))
                        {
                            object_class_names.insert(module.module_reference + "." + assignment.name);
                        }
                        else if (is_defined_object_class(module.module_reference, defined.type_reference,
                                                         object_class_names))
                        {
                            object_class_names.insert(module.module_reference + "." + assignment.name);
                        }
                    }
                }
                if (is_value(assignment) && is_defined(value(assignment).type))
                {
                    const DefinedType& defined = std::get<DefinedType>(value(assignment).type);
                    if (!is_a_parameter(defined.type_reference, assignment.parameters))
                    {
                        const Assignment& inner_assignment = resolve(tree, module.module_reference, defined);
                        if (is_object_class(inner_assignment))
                        {
                            object_class_names.insert(module.module_reference + "." + assignment.name);
                        }
                        else if (is_defined_object_class(module.module_reference, defined.type_reference,
                                                         object_class_names))
                        {
                            object_class_names.insert(module.module_reference + "." + assignment.name);
                        }
                    }
                }
                if (is_object_class(assignment))
                {
                    object_class_names.insert(module.module_reference + "." + assignment.name);
                }
            }
        }

        for (const Module& module : tree.modules)
        {
            for (const Import& import : module.imports)
            {
                for (const std::string& imported_name : import.imported_types)
                {
                    if (is_defined_object_class(import.module_reference, imported_name, object_class_names))
                    {
                        object_class_names.insert(module.module_reference + "." + imported_name);
                    }
                }
            }
        }
    } while (object_class_names.size() > old_number_of_names);
    return object_class_names;
}

// Convert usage of object classes to standard ASN.1 types
void resolve_object_classes(Asn1Tree& tree)
{
    std::unordered_set<std::string> object_class_names = get_object_class_names(tree);

    for (Module& module : tree.modules)
    {
        for (Assignment& assignment : module.assignments)
        {
            if (std::holds_alternative<TypeAssignment>(assignment.specific))
            {

                object_class_to_concrete(tree, module, std::get<TypeAssignment>(assignment.specific).type);
            }
        }
    }

    remove_object_classes(tree, object_class_names);
}
