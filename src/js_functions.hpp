////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "js_types.hpp"
#include "js_class.hpp"

#include "sync/app_service_client.hpp"

namespace realm {
namespace js {

template<typename T>
class Functions : public realm::app::AppServiceClient {

};

template<typename T>
class FunctionsClass : public ClassDefinition<T, realm::js::Functions<T>> {
    using ContextType = typename T::Context;
    using FunctionType = typename T::Function;
    using ObjectType = typename T::Object;
    using ValueType = typename T::Value;
    using String = js::String<T>;
    using Value = js::Value<T>;
    using Object = js::Object<T>;
    using Function = js::Function<T>;
    using ReturnValue = js::ReturnValue<T>;
    using Arguments = js::Arguments<T>;

public:
    std::string name = "Functions";

    static FunctionType create_constructor(ContextType);

    static void call_function(ContextType, ObjectType, Arguments&, ReturnValue&);

    MethodMap<T> const methods = {
        {"_callFunction", wrap<call_function>},
    };
};

template<typename T>
inline typename T::Function FunctionsClass<T>::create_constructor(ContextType ctx) {
    FunctionType functions_constructor = ObjectWrap<T, FunctionsClass<T>>::create_constructor(ctx);
    return functions_constructor;
}

template<typename T>
void FunctionsClass<T>::call_function(ContextType ctx, ObjectType this_object, Arguments &args, ReturnValue &return_value) {
    // arguments: function name, completion callback, function's arguments

    args.validate_count(3);

    auto function_name = Value::validated_to_string(ctx, args[0], "function name");
    auto callback_function = Value::validated_to_function(ctx, args[1], "callback function");
    auto args_json = Value::validated_to_string(ctx, args[2], "arguments as json");

    app::AppServiceClient& functions = *get_internal<app::AppServiceClient>(this_object);

    Protected<typename T::GlobalContext> protected_ctx(Context<T>::get_global_context(ctx));
    Protected<FunctionType> protected_callback(ctx, callback_function);
    Protected<ObjectType> protected_this(ctx, this_object);

    auto callback_handler([=](util::Optional<app::AppError> error, util::Optional<std::string> result) {
        HANDLESCOPE
        if (error) {
            ObjectType error_object = Object::create_empty(protected_ctx);
                        Object::set_property(protected_ctx, object, "message", Value::from_string(protected_ctx, error->message));
            Object::set_property(protected_ctx, object, "code", Value::from_number(protected_ctx, error->error_code.value()));

            ValueType callback_arguments[2];
            callback_arguments[0] = Value::from_undefined(protected_ctx);
            callback_arguments[1] = object;
            Function::callback(protected_ctx, protected_callback, protected_this, 2, callback_arguments);
        }
        else {
            ValueType callback_arguments[2];
            callback_arguments[0] = Value::from_string(protected_ctx, result);
            callback_arguments[1] = Value::from_undefined(protected_ctx);
            Function::callback(protected_ctx, protected_callback, typename T::Object(), 2, callback_arguments);
        }
    });



    functions.call_function(function_name, args_json, service_name, std::move(callback_handler));
}

}
}