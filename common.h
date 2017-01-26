//
// Created by Yegor Jbanov on 1/23/17.
//

#ifndef BARISTA2_COMMON_H
#define BARISTA2_COMMON_H

#define PRIVATE_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = default;      \
  TypeName& operator=(const TypeName&) = default

#endif //BARISTA2_COMMON_H
