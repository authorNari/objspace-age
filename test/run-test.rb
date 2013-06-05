require 'minitest/unit'
$: << File.join(__dir__, '../lib')
$: << File.join(__dir__, '../ext')
$: << __dir__

require 'objspace/age'

MiniTest::Unit.autorun

class TestObjectSpaceAge < MiniTest::Unit::TestCase
  def test_enable
    assert_equal nil, ObjectSpace::Age.enable
  ensure
    ObjectSpace::Age.disable
  end

  def test_where
    ObjectSpace::Age.enable
    a = ''
    assert_equal 'test/run-test.rb:19', ObjectSpace::Age.where(a)
  ensure
    ObjectSpace::Age.disable
  end

  def test_how_old
    ObjectSpace::Age.enable
    a = ''
    GC.start
    assert_equal 1, ObjectSpace::Age.how_old(a)
  ensure
    ObjectSpace::Age.disable
  end

  def test_stats_on_class
    expect = [nil, nil, {Array=>1, String=>10}]
    ObjectSpace::Age.enable
    a = []
    10.times{ a << '' }
    2.times{ GC.start }
    assert_equal expect, ObjectSpace::Age.stats_on(:class)
  ensure
    ObjectSpace::Age.disable
  end

  def test_stats_on_line
    expect = [nil, nil, {"test/run-test.rb:48"=>1, "test/run-test.rb:49"=>10}]
    ObjectSpace::Age.enable
    a = []
    10.times{ a << '' }
    2.times{ GC.start }
    assert_equal expect, ObjectSpace::Age.stats_on(:line)
  ensure
    ObjectSpace::Age.disable
  end
end
