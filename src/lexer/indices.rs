//use super::CompilationUnit;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub(super) struct ByteIdx(usize);

impl ByteIdx {
    pub(super) fn new(i: usize) -> Self {
        Self(i)
    }
    pub(super) fn get(&self) -> usize {
        self.0
    }
}

impl std::ops::Add<usize> for ByteIdx {
    type Output = Self;

    fn add(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

#[derive(Debug, Clone, Copy)]
pub(super) struct ByteSpan {
    start: ByteIdx,
    inclusive_end: ByteIdx,
}

impl ByteSpan {
    pub(super) fn new(start: ByteIdx, inclusive_end: ByteIdx) -> Self {
        Self {
            start,
            inclusive_end,
        }
    }
    pub(super) fn get_start(&self) -> ByteIdx {
        self.start
    }
    pub(super) fn get_inclusive_end(&self) -> ByteIdx {
        self.inclusive_end
    }
    fn merge(&self, other: &Self) -> Self {
        Self {
            start: ByteIdx::new(self.get_start().get().min(other.get_start().get())),
            inclusive_end: ByteIdx::new(
                self.get_inclusive_end()
                    .get()
                    .max(other.get_inclusive_end().get()),
            ),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) struct UcIdx(usize);

impl UcIdx {
    pub(super) fn new(i: usize) -> Self {
        Self(i)
    }
    pub(super) fn get(&self) -> usize {
        self.0
    }
    pub(super) fn get_byte_span(&self, cu: &super::CompilationUnit) -> Option<ByteSpan> {
        cu.ucs.get_byte_span(*self).copied()
    }
}

impl std::ops::Add<usize> for UcIdx {
    type Output = Self;

    fn add(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

impl std::ops::AddAssign<usize> for UcIdx {
    fn add_assign(&mut self, rhs: usize) {
        *self = *self + rhs;
    }
}

impl std::ops::Sub<usize> for UcIdx {
    type Output = Self;

    fn sub(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_sub(rhs).unwrap())
    }
}

impl std::ops::SubAssign<usize> for UcIdx {
    fn sub_assign(&mut self, rhs: usize) {
        *self = *self - rhs;
    }
}

impl std::ops::Sub<UcIdx> for UcIdx {
    type Output = UcSpan;

    fn sub(self, rhs: UcIdx) -> Self::Output {
        assert!(self.get() >= rhs.get());
        UcSpan::new(rhs, self)
    }
}

#[derive(Debug, Clone, Copy)]
pub(super) struct UcSpan {
    start: UcIdx,
    inclusive_end: UcIdx,
}

impl UcSpan {
    pub(super) fn new(start: UcIdx, inclusive_end: UcIdx) -> Self {
        Self {
            start,
            inclusive_end,
        }
    }
    pub(super) fn get_start(&self) -> UcIdx {
        self.start
    }
    pub(super) fn get_inclusive_end(&self) -> UcIdx {
        self.inclusive_end
    }
    pub(super) fn width(&self) -> usize {
        self.get_inclusive_end().get() - self.get_start().get() + 1
    }
    pub(super) fn get_byte_span(&self, cu: &super::CompilationUnit) -> Option<ByteSpan> {
        let start = self.get_start().get_byte_span(cu)?;
        let inclusive_end = self.get_inclusive_end().get_byte_span(cu)?;
        Some(start.merge(&inclusive_end))
    }
}
