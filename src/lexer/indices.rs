#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) struct ByteIndex(usize);

impl ByteIndex {
    pub(super) fn new(i: usize) -> Self {
        Self(i)
    }
    pub(super) fn get(&self) -> usize {
        self.0
    }
}

#[cfg(test)]
mod test_byte_index {
    use super::*;

    #[test]
    fn test_new() {
        for v in [0, usize::MAX, 42] {
            assert_eq!(ByteIndex::new(v).get(), v);
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) struct ByteIndexSpan {
    start: ByteIndex,
    inclusive_end: ByteIndex,
}

impl ByteIndexSpan {
    pub(super) fn new(start: ByteIndex, inclusive_end: ByteIndex) -> Self {
        Self {
            start,
            inclusive_end,
        }
    }
    pub(super) fn get_start(&self) -> ByteIndex {
        self.start
    }
    pub(super) fn get_inclusive_end(&self) -> ByteIndex {
        self.inclusive_end
    }
    pub(super) fn merge(&self, other: &Self) -> Self {
        Self {
            start: ByteIndex::new(self.get_start().get().min(other.get_start().get())),
            inclusive_end: ByteIndex::new(
                self.get_inclusive_end()
                    .get()
                    .max(other.get_inclusive_end().get()),
            ),
        }
    }
}

#[cfg(test)]
mod test_byte_index_span {
    use super::*;

    #[test]
    fn test_raw_content_start_and_end() {
        let start = ByteIndex::new(5);
        let end = ByteIndex::new(10);
        let span = ByteIndexSpan::new(start, end);
        assert_eq!(span.get_start(), start);
        assert_eq!(span.get_inclusive_end(), end);
    }

    #[test]
    fn test_merge() {
        for ((s1, e1), (s2, e2), (s3, e3)) in
            [((5, 10), (8, 15), (5, 15)), ((5, 10), (10, 15), (5, 15))]
        {
            let span1 = ByteIndexSpan::new(ByteIndex::new(s1), ByteIndex::new(e1));
            let span2 = ByteIndexSpan::new(ByteIndex::new(s2), ByteIndex::new(e2));
            let merged = span1.merge(&span2);
            assert_eq!(
                merged,
                ByteIndexSpan::new(ByteIndex::new(s3), ByteIndex::new(e3))
            );
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) struct UcContentIndex(usize);

impl UcContentIndex {
    pub(super) fn new(i: usize) -> Self {
        Self(i)
    }
    pub(super) fn get(&self) -> usize {
        self.0
    }
}

impl std::ops::Add<usize> for UcContentIndex {
    type Output = Self;

    fn add(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_add(rhs).unwrap())
    }
}

impl std::ops::AddAssign<usize> for UcContentIndex {
    fn add_assign(&mut self, rhs: usize) {
        *self = *self + rhs;
    }
}

impl std::ops::Sub<usize> for UcContentIndex {
    type Output = Self;

    fn sub(self, rhs: usize) -> Self::Output {
        Self(self.get().checked_sub(rhs).unwrap())
    }
}

impl std::ops::SubAssign<usize> for UcContentIndex {
    fn sub_assign(&mut self, rhs: usize) {
        *self = *self - rhs;
    }
}

#[cfg(test)]
mod test_uc_content_index {
    use super::*;

    #[test]
    fn test_new_and_get() {
        for v in [0, usize::MAX, 42] {
            assert_eq!(UcContentIndex::new(v).get(), v);
        }
    }

    #[test]
    fn test_add() {
        let index = UcContentIndex::new(5);
        let result = index + 3;
        assert_eq!(result.get(), 8);
    }

    #[test]
    fn test_add_assign() {
        let mut index = UcContentIndex::new(5);
        index += 3;
        assert_eq!(index.get(), 8);
    }

    #[test]
    fn test_sub() {
        let index = UcContentIndex::new(5);
        let result = index - 3;
        assert_eq!(result.get(), 2);
    }

    #[test]
    fn test_sub_assign() {
        let mut index = UcContentIndex::new(5);
        index -= 3;
        assert_eq!(index.get(), 2);
    }
}
