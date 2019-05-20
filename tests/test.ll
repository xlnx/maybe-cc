; ModuleID = 'asd'
source_filename = "asd"

define i32 @main() {
entry:
  %retVal = alloca i32
  %0 = load i32, i32* %retVal
  %a = alloca i32
  store i32 0, i32* %a
  %b = alloca i32
  store i32 1, i32* %b
  %c = alloca i32
  store i32 0, i32* %c
  %1 = load i32, i32* %a
  %2 = load i32, i32* %b
  %3 = add i32 %1, %2
  %4 = load i32, i32* %c
  %5 = add i32 %3, %4
  ret i32 %5
  ret i32 0
}
