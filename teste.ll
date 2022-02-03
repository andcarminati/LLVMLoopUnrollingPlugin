; ModuleID = '../testes/loop1.s'
source_filename = "loop1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@a = dso_local global [50 x i32] zeroinitializer, align 16
@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32 %0, i8** %1) #0 {
  br label %3

3:                                                ; preds = %2, %42
  %.01 = phi i32 [ 0, %2 ], [ %44, %42 ]
  %4 = sext i32 %.01 to i64
  %5 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %4
  store i32 %.01, i32* %5, align 4, !tbaa !2
  %6 = sext i32 %.01 to i64
  %7 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %6
  %8 = load i32, i32* %7, align 4, !tbaa !2
  %9 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %8)
  br label %10

10:                                               ; preds = %3
  %11 = add nsw i32 %.01, 1
  %12 = sext i32 %11 to i64
  %13 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %12
  store i32 %11, i32* %13, align 4, !tbaa !2
  %14 = sext i32 %11 to i64
  %15 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %14
  %16 = load i32, i32* %15, align 4, !tbaa !2
  %17 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %16)
  br label %18

18:                                               ; preds = %10
  %19 = add nsw i32 %11, 1
  %20 = sext i32 %19 to i64
  %21 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %20
  store i32 %19, i32* %21, align 4, !tbaa !2
  %22 = sext i32 %19 to i64
  %23 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %22
  %24 = load i32, i32* %23, align 4, !tbaa !2
  %25 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %24)
  br label %26

26:                                               ; preds = %18
  %27 = add nsw i32 %19, 1
  %28 = sext i32 %27 to i64
  %29 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %28
  store i32 %27, i32* %29, align 4, !tbaa !2
  %30 = sext i32 %27 to i64
  %31 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %30
  %32 = load i32, i32* %31, align 4, !tbaa !2
  %33 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %32)
  br label %34

34:                                               ; preds = %26
  %35 = add nsw i32 %27, 1
  %36 = sext i32 %35 to i64
  %37 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %36
  store i32 %35, i32* %37, align 4, !tbaa !2
  %38 = sext i32 %35 to i64
  %39 = getelementptr inbounds [50 x i32], [50 x i32]* @a, i64 0, i64 %38
  %40 = load i32, i32* %39, align 4, !tbaa !2
  %41 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %40)
  br label %42

42:                                               ; preds = %34
  %43 = mul i32 1, 5
  %44 = add nsw i32 %.01, %43
  %45 = icmp slt i32 %44, 50
  br i1 %45, label %3, label %46, !llvm.loop !6

46:                                               ; preds = %42
  ret i32 0
}

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

declare dso_local i32 @printf(i8*, ...) #2

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nofree nosync nounwind willreturn }
attributes #2 = { "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 12.0.0 (Fedora 12.0.0-2.fc34)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = distinct !{!6, !7, !8}
!7 = !{!"llvm.loop.mustprogress"}
!8 = !{!"llvm.loop.unroll.disable"}
