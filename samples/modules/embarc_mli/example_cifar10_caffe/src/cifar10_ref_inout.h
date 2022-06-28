/*
 * Copyright (c) 2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CIFAR10_REF_INOUT_H_
#define _CIFAR10_REF_INOUT_H_

#include "cifar10_model.h"

#define QMN(type, fraq, val)                                                   \
	(type)(val * (1u << (fraq)) + ((val >= 0) ? 0.5f : -0.5f))
#define FRQ_BITS(int_part, el_type) ((sizeof(el_type) * 8) - int_part - 1)

#define INQ(val) ((unsigned char)val)
#define PBQ(val) (val)
/* -- Model input vector #12: Compile-time quantization - */
#define IN_IMG_12_SHAPE                                                        \
	{                                                                      \
		32, 32, 3                                                      \
	}
#define IN_IMG_12_RANK (3)
#define IN_IMG_12                                                              \
	{                                                                      \
		INQ(91), INQ(64), INQ(30), INQ(82), INQ(58), INQ(30), INQ(87), \
			INQ(73), INQ(59), INQ(89), INQ(87), INQ(83), INQ(95),  \
			INQ(92), INQ(80), INQ(98), INQ(90), INQ(71), INQ(97),  \
			INQ(87), INQ(68), INQ(94), INQ(90), INQ(75), INQ(92),  \
			INQ(93), INQ(84), INQ(98), INQ(94), INQ(85), INQ(81),  \
			INQ(76), INQ(66), INQ(28), INQ(26), INQ(19), INQ(15),  \
			INQ(15), INQ(10), INQ(18), INQ(19), INQ(16), INQ(19),  \
			INQ(19), INQ(20), INQ(19), INQ(19), INQ(20), INQ(16),  \
			INQ(16), INQ(17), INQ(15), INQ(15), INQ(16), INQ(13),  \
			INQ(15), INQ(13), INQ(10), INQ(16), INQ(8), INQ(10),   \
			INQ(14), INQ(10), INQ(13), INQ(13), INQ(12), INQ(15),  \
			INQ(14), INQ(8), INQ(26), INQ(24), INQ(12), INQ(29),   \
			INQ(25), INQ(15), INQ(26), INQ(21), INQ(12), INQ(39),  \
			INQ(34), INQ(25), INQ(25), INQ(20), INQ(11), INQ(18),  \
			INQ(13), INQ(6), INQ(22), INQ(13), INQ(13), INQ(22),   \
			INQ(15), INQ(13), INQ(18), INQ(13), INQ(10), INQ(94),  \
			INQ(65), INQ(34), INQ(79), INQ(56), INQ(30), INQ(90),  \
			INQ(78), INQ(59), INQ(124), INQ(120), INQ(106),        \
			INQ(144), INQ(138), INQ(118), INQ(133), INQ(121),      \
			INQ(96), INQ(111), INQ(97), INQ(71), INQ(101),         \
			INQ(94), INQ(71), INQ(98), INQ(96), INQ(81), INQ(94),  \
			INQ(88), INQ(79), INQ(48), INQ(41), INQ(35), INQ(14),  \
			INQ(9), INQ(6), INQ(12), INQ(10), INQ(7), INQ(11),     \
			INQ(11), INQ(9), INQ(14), INQ(13), INQ(14), INQ(16),   \
			INQ(14), INQ(16), INQ(24), INQ(23), INQ(24), INQ(32),  \
			INQ(30), INQ(32), INQ(26), INQ(26), INQ(26), INQ(27),  \
			INQ(28), INQ(24), INQ(22), INQ(22), INQ(20), INQ(22),  \
			INQ(19), INQ(15), INQ(52), INQ(48), INQ(32), INQ(67),  \
			INQ(61), INQ(36), INQ(56), INQ(45), INQ(24), INQ(57),  \
			INQ(45), INQ(24), INQ(67), INQ(55), INQ(34), INQ(60),  \
			INQ(49), INQ(26), INQ(42), INQ(29), INQ(10), INQ(36),  \
			INQ(19), INQ(7), INQ(61), INQ(46), INQ(32), INQ(47),   \
			INQ(36), INQ(26), INQ(94), INQ(66), INQ(39), INQ(86),  \
			INQ(65), INQ(40), INQ(120), INQ(105), INQ(82),         \
			INQ(156), INQ(142), INQ(119), INQ(155), INQ(138),      \
			INQ(111), INQ(147), INQ(125), INQ(93), INQ(135),       \
			INQ(112), INQ(79), INQ(123), INQ(105), INQ(76),        \
			INQ(114), INQ(103), INQ(81), INQ(88), INQ(78),         \
			INQ(65), INQ(41), INQ(32), INQ(22), INQ(39), INQ(32),  \
			INQ(25), INQ(42), INQ(38), INQ(32), INQ(40), INQ(37),  \
			INQ(31), INQ(49), INQ(48), INQ(32), INQ(51), INQ(50),  \
			INQ(33), INQ(58), INQ(57), INQ(41), INQ(56), INQ(55),  \
			INQ(38), INQ(47), INQ(45), INQ(31), INQ(30), INQ(28),  \
			INQ(21), INQ(38), INQ(33), INQ(28), INQ(29), INQ(21),  \
			INQ(14), INQ(53), INQ(44), INQ(27), INQ(55), INQ(45),  \
			INQ(21), INQ(41), INQ(26), INQ(7), INQ(71), INQ(54),   \
			INQ(29), INQ(53), INQ(36), INQ(12), INQ(63), INQ(47),  \
			INQ(20), INQ(63), INQ(45), INQ(21), INQ(54), INQ(33),  \
			INQ(14), INQ(82), INQ(64), INQ(42), INQ(59), INQ(46),  \
			INQ(32), INQ(89), INQ(69), INQ(42), INQ(108), INQ(90), \
			INQ(67), INQ(136), INQ(112), INQ(88), INQ(133),        \
			INQ(101), INQ(72), INQ(130), INQ(95), INQ(63),         \
			INQ(126), INQ(88), INQ(52), INQ(124), INQ(85),         \
			INQ(48), INQ(126), INQ(92), INQ(60), INQ(124),         \
			INQ(100), INQ(74), INQ(85), INQ(72), INQ(51), INQ(47), \
			INQ(38), INQ(19), INQ(54), INQ(48), INQ(30), INQ(61),  \
			INQ(56), INQ(41), INQ(94), INQ(90), INQ(75), INQ(84),  \
			INQ(81), INQ(59), INQ(84), INQ(81), INQ(57), INQ(61),  \
			INQ(58), INQ(35), INQ(69), INQ(66), INQ(43), INQ(74),  \
			INQ(71), INQ(48), INQ(45), INQ(39), INQ(24), INQ(48),  \
			INQ(39), INQ(28), INQ(32), INQ(20), INQ(11), INQ(37),  \
			INQ(23), INQ(11), INQ(54), INQ(40), INQ(20), INQ(56),  \
			INQ(39), INQ(20), INQ(73), INQ(56), INQ(35), INQ(58),  \
			INQ(41), INQ(21), INQ(44), INQ(27), INQ(9), INQ(66),   \
			INQ(46), INQ(24), INQ(80), INQ(56), INQ(34), INQ(72),  \
			INQ(54), INQ(33), INQ(32), INQ(22), INQ(8), INQ(72),   \
			INQ(60), INQ(33), INQ(94), INQ(78), INQ(56), INQ(105), \
			INQ(75), INQ(51), INQ(106), INQ(62), INQ(33),          \
			INQ(134), INQ(86), INQ(53), INQ(123), INQ(73),         \
			INQ(36), INQ(109), INQ(59), INQ(20), INQ(117),         \
			INQ(70), INQ(36), INQ(113), INQ(77), INQ(47), INQ(88), \
			INQ(70), INQ(41), INQ(61), INQ(51), INQ(23), INQ(64),  \
			INQ(57), INQ(32), INQ(75), INQ(69), INQ(47), INQ(99),  \
			INQ(94), INQ(74), INQ(72), INQ(66), INQ(51), INQ(82),  \
			INQ(76), INQ(63), INQ(69), INQ(63), INQ(50), INQ(49),  \
			INQ(43), INQ(30), INQ(48), INQ(42), INQ(27), INQ(75),  \
			INQ(68), INQ(45), INQ(64), INQ(54), INQ(36), INQ(47),  \
			INQ(33), INQ(23), INQ(62), INQ(46), INQ(31), INQ(68),  \
			INQ(52), INQ(31), INQ(54), INQ(37), INQ(21), INQ(47),  \
			INQ(31), INQ(17), INQ(56), INQ(39), INQ(26), INQ(40),  \
			INQ(24), INQ(10), INQ(32), INQ(15), INQ(4), INQ(46),   \
			INQ(25), INQ(14), INQ(36), INQ(20), INQ(8), INQ(25),   \
			INQ(19), INQ(5), INQ(43), INQ(37), INQ(16), INQ(54),   \
			INQ(39), INQ(20), INQ(62), INQ(35), INQ(16), INQ(83),  \
			INQ(45), INQ(24), INQ(127), INQ(81), INQ(51),          \
			INQ(123), INQ(73), INQ(39), INQ(105), INQ(53),         \
			INQ(17), INQ(110), INQ(57), INQ(21), INQ(102),         \
			INQ(53), INQ(16), INQ(97), INQ(65), INQ(25), INQ(79),  \
			INQ(59), INQ(27), INQ(66), INQ(56), INQ(33), INQ(87),  \
			INQ(81), INQ(62), INQ(75), INQ(69), INQ(53), INQ(86),  \
			INQ(80), INQ(66), INQ(80), INQ(74), INQ(60), INQ(67),  \
			INQ(60), INQ(46), INQ(71), INQ(65), INQ(52), INQ(77),  \
			INQ(72), INQ(55), INQ(88), INQ(82), INQ(58), INQ(66),  \
			INQ(58), INQ(41), INQ(50), INQ(40), INQ(30), INQ(62),  \
			INQ(53), INQ(37), INQ(62), INQ(52), INQ(30), INQ(58),  \
			INQ(42), INQ(24), INQ(31), INQ(15), INQ(3), INQ(40),   \
			INQ(26), INQ(10), INQ(38), INQ(26), INQ(11), INQ(31),  \
			INQ(20), INQ(8), INQ(23), INQ(10), INQ(6), INQ(29),    \
			INQ(15), INQ(8), INQ(27), INQ(16), INQ(6), INQ(47),    \
			INQ(42), INQ(26), INQ(46), INQ(29), INQ(11), INQ(47),  \
			INQ(19), INQ(3), INQ(67), INQ(30), INQ(11), INQ(111),  \
			INQ(66), INQ(36), INQ(116), INQ(69), INQ(35),          \
			INQ(101), INQ(51), INQ(15), INQ(105), INQ(52),         \
			INQ(14), INQ(105), INQ(51), INQ(12), INQ(107),         \
			INQ(61), INQ(21), INQ(89), INQ(58), INQ(27), INQ(56),  \
			INQ(40), INQ(20), INQ(56), INQ(49), INQ(33), INQ(67),  \
			INQ(63), INQ(46), INQ(77), INQ(71), INQ(55), INQ(68),  \
			INQ(62), INQ(46), INQ(74), INQ(68), INQ(51), INQ(75),  \
			INQ(69), INQ(53), INQ(78), INQ(72), INQ(54), INQ(76),  \
			INQ(70), INQ(48), INQ(49), INQ(42), INQ(26), INQ(52),  \
			INQ(44), INQ(32), INQ(52), INQ(45), INQ(29), INQ(51),  \
			INQ(43), INQ(20), INQ(72), INQ(56), INQ(34), INQ(59),  \
			INQ(43), INQ(24), INQ(63), INQ(50), INQ(30), INQ(56),  \
			INQ(44), INQ(27), INQ(37), INQ(28), INQ(13), INQ(28),  \
			INQ(20), INQ(6), INQ(45), INQ(33), INQ(13), INQ(34),   \
			INQ(22), INQ(5), INQ(69), INQ(60), INQ(46), INQ(71),   \
			INQ(50), INQ(31), INQ(68), INQ(35), INQ(17), INQ(66),  \
			INQ(25), INQ(2), INQ(98), INQ(54), INQ(22), INQ(109),  \
			INQ(64), INQ(30), INQ(102), INQ(54), INQ(17),          \
			INQ(110), INQ(60), INQ(21), INQ(112), INQ(59),         \
			INQ(19), INQ(111), INQ(55), INQ(19), INQ(97), INQ(53), \
			INQ(26), INQ(62), INQ(40), INQ(21), INQ(48), INQ(42),  \
			INQ(25), INQ(52), INQ(50), INQ(32), INQ(60), INQ(55),  \
			INQ(39), INQ(82), INQ(76), INQ(60), INQ(75), INQ(69),  \
			INQ(53), INQ(57), INQ(51), INQ(35), INQ(46), INQ(40),  \
			INQ(23), INQ(81), INQ(75), INQ(54), INQ(60), INQ(52),  \
			INQ(36), INQ(41), INQ(33), INQ(22), INQ(55), INQ(48),  \
			INQ(31), INQ(41), INQ(33), INQ(14), INQ(39), INQ(23),  \
			INQ(6), INQ(61), INQ(45), INQ(24), INQ(55), INQ(42),   \
			INQ(24), INQ(49), INQ(38), INQ(21), INQ(40), INQ(31),  \
			INQ(15), INQ(29), INQ(21), INQ(5), INQ(49), INQ(36),   \
			INQ(11), INQ(45), INQ(32), INQ(8), INQ(79), INQ(60),   \
			INQ(44), INQ(112), INQ(87), INQ(67), INQ(134),         \
			INQ(100), INQ(78), INQ(104), INQ(65), INQ(39),         \
			INQ(114), INQ(73), INQ(40), INQ(118), INQ(75),         \
			INQ(40), INQ(109), INQ(64), INQ(27), INQ(111),         \
			INQ(64), INQ(24), INQ(108), INQ(57), INQ(17),          \
			INQ(103), INQ(48), INQ(12), INQ(95), INQ(52), INQ(25), \
			INQ(66), INQ(44), INQ(26), INQ(56), INQ(50), INQ(34),  \
			INQ(59), INQ(57), INQ(40), INQ(82), INQ(77), INQ(60),  \
			INQ(75), INQ(69), INQ(53), INQ(75), INQ(69), INQ(53),  \
			INQ(68), INQ(62), INQ(46), INQ(73), INQ(67), INQ(51),  \
			INQ(73), INQ(67), INQ(46), INQ(64), INQ(57), INQ(40),  \
			INQ(42), INQ(34), INQ(23), INQ(52), INQ(45), INQ(29),  \
			INQ(44), INQ(36), INQ(16), INQ(50), INQ(34), INQ(15),  \
			INQ(53), INQ(37), INQ(16), INQ(42), INQ(29), INQ(11),  \
			INQ(45), INQ(34), INQ(17), INQ(52), INQ(41), INQ(26),  \
			INQ(42), INQ(29), INQ(18), INQ(44), INQ(27), INQ(13),  \
			INQ(38), INQ(21), INQ(8), INQ(120), INQ(87), INQ(66),  \
			INQ(129), INQ(98), INQ(77), INQ(158), INQ(128),        \
			INQ(107), INQ(136), INQ(105), INQ(83), INQ(127),       \
			INQ(90), INQ(58), INQ(116), INQ(76), INQ(39),          \
			INQ(109), INQ(66), INQ(28), INQ(114), INQ(69),         \
			INQ(28), INQ(106), INQ(59), INQ(17), INQ(100),         \
			INQ(54), INQ(14), INQ(114), INQ(81), INQ(50), INQ(93), \
			INQ(77), INQ(57), INQ(65), INQ(58), INQ(43), INQ(63),  \
			INQ(58), INQ(42), INQ(72), INQ(66), INQ(51), INQ(71),  \
			INQ(65), INQ(50), INQ(99), INQ(93), INQ(77), INQ(95),  \
			INQ(89), INQ(73), INQ(107), INQ(101), INQ(83),         \
			INQ(100), INQ(94), INQ(71), INQ(64), INQ(57), INQ(41), \
			INQ(46), INQ(38), INQ(27), INQ(40), INQ(34), INQ(18),  \
			INQ(45), INQ(37), INQ(15), INQ(51), INQ(34), INQ(14),  \
			INQ(54), INQ(38), INQ(17), INQ(43), INQ(29), INQ(11),  \
			INQ(40), INQ(29), INQ(14), INQ(49), INQ(39), INQ(24),  \
			INQ(53), INQ(39), INQ(20), INQ(53), INQ(33), INQ(16),  \
			INQ(36), INQ(17), INQ(5), INQ(161), INQ(120), INQ(94), \
			INQ(126), INQ(93), INQ(70), INQ(135), INQ(108),        \
			INQ(90), INQ(143), INQ(117), INQ(100), INQ(144),       \
			INQ(108), INQ(79), INQ(126), INQ(86), INQ(52),         \
			INQ(102), INQ(62), INQ(26), INQ(110), INQ(68),         \
			INQ(30), INQ(105), INQ(61), INQ(21), INQ(118),         \
			INQ(86), INQ(46), INQ(170), INQ(149), INQ(118),        \
			INQ(156), INQ(146), INQ(124), INQ(115), INQ(110),      \
			INQ(92), INQ(114), INQ(107), INQ(90), INQ(123),        \
			INQ(117), INQ(100), INQ(126), INQ(120), INQ(104),      \
			INQ(152), INQ(146), INQ(130), INQ(144), INQ(137),      \
			INQ(122), INQ(114), INQ(107), INQ(90), INQ(113),       \
			INQ(107), INQ(85), INQ(115), INQ(108), INQ(91),        \
			INQ(120), INQ(112), INQ(100), INQ(109), INQ(102),      \
			INQ(85), INQ(85), INQ(77), INQ(57), INQ(81), INQ(65),  \
			INQ(46), INQ(67), INQ(52), INQ(31), INQ(51), INQ(38),  \
			INQ(18), INQ(54), INQ(40), INQ(22), INQ(54), INQ(40),  \
			INQ(19), INQ(60), INQ(44), INQ(16), INQ(82), INQ(63),  \
			INQ(31), INQ(76), INQ(57), INQ(29), INQ(164),          \
			INQ(126), INQ(100), INQ(130), INQ(103), INQ(78),       \
			INQ(99), INQ(72), INQ(51), INQ(129), INQ(94), INQ(76), \
			INQ(159), INQ(114), INQ(93), INQ(152), INQ(111),       \
			INQ(90), INQ(123), INQ(91), INQ(69), INQ(112),         \
			INQ(80), INQ(54), INQ(118), INQ(83), INQ(53),          \
			INQ(151), INQ(131), INQ(107), INQ(189), INQ(178),      \
			INQ(159), INQ(173), INQ(167), INQ(148), INQ(149),      \
			INQ(145), INQ(122), INQ(154), INQ(146), INQ(122),      \
			INQ(178), INQ(165), INQ(147), INQ(177), INQ(169),      \
			INQ(152), INQ(175), INQ(170), INQ(152), INQ(170),      \
			INQ(162), INQ(146), INQ(140), INQ(128), INQ(114),      \
			INQ(141), INQ(134), INQ(117), INQ(172), INQ(165),      \
			INQ(149), INQ(179), INQ(171), INQ(156), INQ(178),      \
			INQ(171), INQ(155), INQ(172), INQ(164), INQ(148),      \
			INQ(159), INQ(149), INQ(135), INQ(127), INQ(118),      \
			INQ(99), INQ(100), INQ(87), INQ(62), INQ(97), INQ(72), \
			INQ(44), INQ(89), INQ(59), INQ(31), INQ(59), INQ(35),  \
			INQ(12), INQ(63), INQ(41), INQ(18), INQ(87), INQ(69),  \
			INQ(44), INQ(161), INQ(125), INQ(95), INQ(147),        \
			INQ(116), INQ(88), INQ(125), INQ(89), INQ(65),         \
			INQ(144), INQ(100), INQ(80), INQ(146), INQ(105),       \
			INQ(87), INQ(141), INQ(111), INQ(95), INQ(161),        \
			INQ(138), INQ(124), INQ(156), INQ(133), INQ(115),      \
			INQ(169), INQ(145), INQ(122), INQ(186), INQ(170),      \
			INQ(153), INQ(182), INQ(171), INQ(153), INQ(162),      \
			INQ(151), INQ(131), INQ(147), INQ(138), INQ(113),      \
			INQ(161), INQ(150), INQ(124), INQ(173), INQ(155),      \
			INQ(136), INQ(174), INQ(162), INQ(144), INQ(178),      \
			INQ(173), INQ(154), INQ(178), INQ(170), INQ(154),      \
			INQ(172), INQ(160), INQ(146), INQ(174), INQ(166),      \
			INQ(150), INQ(183), INQ(176), INQ(160), INQ(173),      \
			INQ(166), INQ(150), INQ(169), INQ(162), INQ(146),      \
			INQ(184), INQ(176), INQ(162), INQ(178), INQ(169),      \
			INQ(158), INQ(172), INQ(164), INQ(147), INQ(145),      \
			INQ(133), INQ(108), INQ(115), INQ(89), INQ(60),        \
			INQ(99), INQ(67), INQ(38), INQ(83), INQ(60), INQ(37),  \
			INQ(66), INQ(49), INQ(28), INQ(53), INQ(40), INQ(21),  \
			INQ(163), INQ(124), INQ(89), INQ(159), INQ(119),       \
			INQ(88), INQ(163), INQ(118), INQ(92), INQ(166),        \
			INQ(120), INQ(98), INQ(97), INQ(73), INQ(52), INQ(81), \
			INQ(66), INQ(52), INQ(155), INQ(142), INQ(132),        \
			INQ(180), INQ(167), INQ(155), INQ(195), INQ(181),      \
			INQ(164), INQ(188), INQ(174), INQ(157), INQ(154),      \
			INQ(136), INQ(115), INQ(131), INQ(110), INQ(85),       \
			INQ(137), INQ(118), INQ(92), INQ(164), INQ(146),       \
			INQ(121), INQ(147), INQ(125), INQ(102), INQ(150),      \
			INQ(135), INQ(112), INQ(183), INQ(176), INQ(155),      \
			INQ(186), INQ(178), INQ(162), INQ(185), INQ(174),      \
			INQ(162), INQ(186), INQ(178), INQ(163), INQ(189),      \
			INQ(183), INQ(166), INQ(182), INQ(175), INQ(159),      \
			INQ(169), INQ(163), INQ(146), INQ(182), INQ(174),      \
			INQ(160), INQ(182), INQ(172), INQ(162), INQ(178),      \
			INQ(170), INQ(156), INQ(179), INQ(169), INQ(148),      \
			INQ(145), INQ(123), INQ(100), INQ(122), INQ(94),       \
			INQ(70), INQ(139), INQ(121), INQ(96), INQ(125),        \
			INQ(113), INQ(90), INQ(44), INQ(38), INQ(21),          \
			INQ(168), INQ(126), INQ(92), INQ(172), INQ(128),       \
			INQ(99), INQ(172), INQ(128), INQ(102), INQ(146),       \
			INQ(110), INQ(84), INQ(59), INQ(51), INQ(23), INQ(51), \
			INQ(48), INQ(28), INQ(138), INQ(130), INQ(120),        \
			INQ(177), INQ(169), INQ(160), INQ(192), INQ(186),      \
			INQ(172), INQ(169), INQ(158), INQ(139), INQ(126),      \
			INQ(104), INQ(78), INQ(124), INQ(93), INQ(62),         \
			INQ(139), INQ(110), INQ(81), INQ(153), INQ(127),       \
			INQ(103), INQ(133), INQ(106), INQ(79), INQ(139),       \
			INQ(120), INQ(93), INQ(184), INQ(175), INQ(152),       \
			INQ(193), INQ(185), INQ(169), INQ(193), INQ(183),      \
			INQ(172), INQ(195), INQ(187), INQ(172), INQ(198),      \
			INQ(191), INQ(175), INQ(198), INQ(191), INQ(175),      \
			INQ(190), INQ(183), INQ(167), INQ(188), INQ(179),      \
			INQ(166), INQ(191), INQ(180), INQ(171), INQ(194),      \
			INQ(186), INQ(174), INQ(199), INQ(191), INQ(175),      \
			INQ(179), INQ(161), INQ(144), INQ(146), INQ(122),      \
			INQ(103), INQ(167), INQ(147), INQ(125), INQ(181),      \
			INQ(169), INQ(146), INQ(99), INQ(97), INQ(74),         \
			INQ(177), INQ(129), INQ(103), INQ(178), INQ(133),      \
			INQ(110), INQ(168), INQ(133), INQ(110), INQ(118),      \
			INQ(99), INQ(71), INQ(57), INQ(63), INQ(26), INQ(43),  \
			INQ(46), INQ(18), INQ(114), INQ(106), INQ(92),         \
			INQ(173), INQ(165), INQ(154), INQ(192), INQ(189),      \
			INQ(174), INQ(163), INQ(153), INQ(133), INQ(131),      \
			INQ(103), INQ(73), INQ(149), INQ(108), INQ(70),        \
			INQ(149), INQ(109), INQ(78), INQ(142), INQ(110),       \
			INQ(87), INQ(133), INQ(100), INQ(71), INQ(156),        \
			INQ(133), INQ(103), INQ(195), INQ(184), INQ(159),      \
			INQ(197), INQ(190), INQ(173), INQ(198), INQ(189),      \
			INQ(179), INQ(200), INQ(192), INQ(177), INQ(203),      \
			INQ(196), INQ(180), INQ(205), INQ(198), INQ(182),      \
			INQ(195), INQ(188), INQ(172), INQ(192), INQ(182),      \
			INQ(169), INQ(201), INQ(188), INQ(181), INQ(204),      \
			INQ(196), INQ(186), INQ(203), INQ(197), INQ(185),      \
			INQ(189), INQ(175), INQ(163), INQ(140), INQ(117),      \
			INQ(106), INQ(137), INQ(111), INQ(95), INQ(170),       \
			INQ(154), INQ(130), INQ(129), INQ(127), INQ(96),       \
			INQ(182), INQ(135), INQ(115), INQ(177), INQ(136),      \
			INQ(116), INQ(153), INQ(129), INQ(104), INQ(96),       \
			INQ(94), INQ(61), INQ(70), INQ(85), INQ(41), INQ(58),  \
			INQ(66), INQ(30), INQ(98), INQ(91), INQ(72), INQ(164), \
			INQ(155), INQ(142), INQ(192), INQ(187), INQ(172),      \
			INQ(180), INQ(171), INQ(151), INQ(169), INQ(144),      \
			INQ(115), INQ(172), INQ(131), INQ(95), INQ(154),       \
			INQ(111), INQ(80), INQ(140), INQ(103), INQ(77),        \
			INQ(137), INQ(101), INQ(67), INQ(176), INQ(152),       \
			INQ(121), INQ(201), INQ(191), INQ(168), INQ(197),      \
			INQ(189), INQ(173), INQ(200), INQ(190), INQ(177),      \
			INQ(200), INQ(192), INQ(177), INQ(203), INQ(197),      \
			INQ(180), INQ(204), INQ(197), INQ(181), INQ(191),      \
			INQ(185), INQ(168), INQ(191), INQ(183), INQ(169),      \
			INQ(202), INQ(190), INQ(184), INQ(204), INQ(197),      \
			INQ(188), INQ(204), INQ(199), INQ(189), INQ(189),      \
			INQ(180), INQ(171), INQ(145), INQ(127), INQ(117),      \
			INQ(115), INQ(86), INQ(69), INQ(129), INQ(106),        \
			INQ(81), INQ(113), INQ(103), INQ(68), INQ(182),        \
			INQ(142), INQ(123), INQ(165), INQ(133), INQ(110),      \
			INQ(121), INQ(109), INQ(77), INQ(83), INQ(93),         \
			INQ(51), INQ(73), INQ(93), INQ(45), INQ(62), INQ(77),  \
			INQ(35), INQ(76), INQ(79), INQ(51), INQ(152),          \
			INQ(143), INQ(129), INQ(193), INQ(181), INQ(172),      \
			INQ(201), INQ(195), INQ(179), INQ(198), INQ(187),      \
			INQ(165), INQ(181), INQ(157), INQ(131), INQ(160),      \
			INQ(128), INQ(101), INQ(147), INQ(110), INQ(79),       \
			INQ(140), INQ(103), INQ(63), INQ(176), INQ(154),       \
			INQ(125), INQ(195), INQ(188), INQ(173), INQ(194),      \
			INQ(184), INQ(171), INQ(196), INQ(180), INQ(163),      \
			INQ(193), INQ(185), INQ(169), INQ(197), INQ(191),      \
			INQ(175), INQ(193), INQ(187), INQ(171), INQ(191),      \
			INQ(185), INQ(169), INQ(197), INQ(190), INQ(176),      \
			INQ(202), INQ(195), INQ(186), INQ(201), INQ(195),      \
			INQ(186), INQ(198), INQ(192), INQ(182), INQ(183),      \
			INQ(177), INQ(166), INQ(145), INQ(133), INQ(117),      \
			INQ(117), INQ(90), INQ(65), INQ(131), INQ(97),         \
			INQ(71), INQ(123), INQ(95), INQ(66), INQ(182),         \
			INQ(144), INQ(125), INQ(138), INQ(114), INQ(88),       \
			INQ(89), INQ(89), INQ(51), INQ(75), INQ(95), INQ(48),  \
			INQ(71), INQ(94), INQ(44), INQ(61), INQ(77), INQ(34),  \
			INQ(44), INQ(50), INQ(20), INQ(113), INQ(108),         \
			INQ(90), INQ(183), INQ(171), INQ(161), INQ(205),       \
			INQ(197), INQ(186), INQ(206), INQ(198), INQ(183),      \
			INQ(193), INQ(179), INQ(160), INQ(183), INQ(165),      \
			INQ(142), INQ(163), INQ(140), INQ(112), INQ(141),      \
			INQ(111), INQ(77), INQ(167), INQ(148), INQ(125),       \
			INQ(183), INQ(176), INQ(168), INQ(183), INQ(172),      \
			INQ(164), INQ(185), INQ(169), INQ(156), INQ(189),      \
			INQ(181), INQ(165), INQ(190), INQ(184), INQ(168),      \
			INQ(185), INQ(179), INQ(163), INQ(194), INQ(188),      \
			INQ(172), INQ(199), INQ(194), INQ(179), INQ(200),      \
			INQ(195), INQ(182), INQ(196), INQ(188), INQ(175),      \
			INQ(183), INQ(170), INQ(156), INQ(166), INQ(147),      \
			INQ(133), INQ(144), INQ(120), INQ(101), INQ(137),      \
			INQ(103), INQ(77), INQ(150), INQ(108), INQ(83),        \
			INQ(138), INQ(100), INQ(74), INQ(161), INQ(128),       \
			INQ(108), INQ(103), INQ(91), INQ(60), INQ(75),         \
			INQ(86), INQ(44), INQ(69), INQ(95), INQ(46), INQ(68),  \
			INQ(91), INQ(43), INQ(69), INQ(85), INQ(42), INQ(46),  \
			INQ(54), INQ(22), INQ(53), INQ(54), INQ(31), INQ(138), \
			INQ(133), INQ(116), INQ(197), INQ(187), INQ(178),      \
			INQ(205), INQ(195), INQ(186), INQ(194), INQ(186),      \
			INQ(172), INQ(187), INQ(181), INQ(162), INQ(178),      \
			INQ(170), INQ(148), INQ(164), INQ(144), INQ(120),      \
			INQ(168), INQ(154), INQ(138), INQ(169), INQ(162),      \
			INQ(156), INQ(167), INQ(158), INQ(153), INQ(182),      \
			INQ(168), INQ(160), INQ(188), INQ(181), INQ(166),      \
			INQ(178), INQ(172), INQ(156), INQ(182), INQ(176),      \
			INQ(160), INQ(197), INQ(190), INQ(174), INQ(199),      \
			INQ(195), INQ(178), INQ(197), INQ(194), INQ(177),      \
			INQ(184), INQ(174), INQ(156), INQ(154), INQ(130),      \
			INQ(112), INQ(149), INQ(113), INQ(94), INQ(162),       \
			INQ(120), INQ(99), INQ(162), INQ(119), INQ(95),        \
			INQ(163), INQ(116), INQ(93), INQ(148), INQ(105),       \
			INQ(81), INQ(128), INQ(108), INQ(83), INQ(83),         \
			INQ(83), INQ(48), INQ(71), INQ(89), INQ(45), INQ(68),  \
			INQ(93), INQ(45), INQ(68), INQ(89), INQ(42), INQ(70),  \
			INQ(87), INQ(43), INQ(68), INQ(80), INQ(43), INQ(44),  \
			INQ(51), INQ(20), INQ(77), INQ(78), INQ(53), INQ(173), \
			INQ(165), INQ(152), INQ(197), INQ(188), INQ(178),      \
			INQ(195), INQ(189), INQ(176), INQ(177), INQ(174),      \
			INQ(159), INQ(164), INQ(161), INQ(144), INQ(164),      \
			INQ(154), INQ(134), INQ(158), INQ(149), INQ(134),      \
			INQ(153), INQ(146), INQ(136), INQ(153), INQ(145),      \
			INQ(137), INQ(161), INQ(152), INQ(143), INQ(162),      \
			INQ(156), INQ(141), INQ(152), INQ(146), INQ(130),      \
			INQ(171), INQ(166), INQ(149), INQ(197), INQ(191),      \
			INQ(175), INQ(199), INQ(194), INQ(178), INQ(200),      \
			INQ(195), INQ(180), INQ(172), INQ(159), INQ(142),      \
			INQ(128), INQ(101), INQ(82), INQ(150), INQ(110),       \
			INQ(88), INQ(177), INQ(130), INQ(107), INQ(177),       \
			INQ(130), INQ(107), INQ(174), INQ(127), INQ(105),      \
			INQ(159), INQ(114), INQ(91), INQ(97), INQ(92),         \
			INQ(62), INQ(75), INQ(87), INQ(48), INQ(71), INQ(90),  \
			INQ(45), INQ(73), INQ(91), INQ(45), INQ(73), INQ(90),  \
			INQ(46), INQ(73), INQ(90), INQ(46), INQ(74), INQ(89),  \
			INQ(48), INQ(68), INQ(80), INQ(42), INQ(46), INQ(53),  \
			INQ(23), INQ(135), INQ(130), INQ(110), INQ(191),       \
			INQ(182), INQ(166), INQ(197), INQ(189), INQ(173),      \
			INQ(170), INQ(161), INQ(146), INQ(132), INQ(124),      \
			INQ(109), INQ(109), INQ(106), INQ(87), INQ(107),       \
			INQ(103), INQ(84), INQ(113), INQ(106), INQ(90),        \
			INQ(100), INQ(93), INQ(80), INQ(95), INQ(89), INQ(77), \
			INQ(96), INQ(89), INQ(75), INQ(81), INQ(74), INQ(60),  \
			INQ(127), INQ(121), INQ(106), INQ(188), INQ(182),      \
			INQ(166), INQ(196), INQ(190), INQ(176), INQ(204),      \
			INQ(194), INQ(188), INQ(175), INQ(159), INQ(150),      \
			INQ(116), INQ(89), INQ(73), INQ(135), INQ(98),         \
			INQ(77), INQ(178), INQ(134), INQ(110), INQ(182),       \
			INQ(137), INQ(114), INQ(183), INQ(141), INQ(117),      \
			INQ(175), INQ(136), INQ(111), INQ(79), INQ(87),        \
			INQ(50), INQ(70), INQ(88), INQ(46), INQ(69), INQ(89),  \
			INQ(44), INQ(74), INQ(91), INQ(46), INQ(76), INQ(92),  \
			INQ(48), INQ(75), INQ(91), INQ(48), INQ(76), INQ(91),  \
			INQ(50), INQ(74), INQ(89), INQ(48), INQ(46), INQ(57),  \
			INQ(20), INQ(105), INQ(102), INQ(79), INQ(190),        \
			INQ(182), INQ(164), INQ(198), INQ(188), INQ(171),      \
			INQ(168), INQ(155), INQ(141), INQ(119), INQ(108),      \
			INQ(93), INQ(66), INQ(70), INQ(45), INQ(55), INQ(60),  \
			INQ(32), INQ(63), INQ(65), INQ(36), INQ(61), INQ(63),  \
			INQ(35), INQ(53), INQ(57), INQ(30), INQ(60), INQ(63),  \
			INQ(37), INQ(56), INQ(57), INQ(32), INQ(65), INQ(62),  \
			INQ(41), INQ(152), INQ(145), INQ(127), INQ(193),       \
			INQ(184), INQ(172), INQ(202), INQ(191), INQ(185),      \
			INQ(199), INQ(185), INQ(175), INQ(148), INQ(128),      \
			INQ(112), INQ(121), INQ(97), INQ(76), INQ(161),        \
			INQ(131), INQ(105), INQ(183), INQ(146), INQ(119),      \
			INQ(177), INQ(141), INQ(113), INQ(173), INQ(136),      \
			INQ(110), INQ(72), INQ(90), INQ(43), INQ(69), INQ(88), \
			INQ(41), INQ(69), INQ(90), INQ(42), INQ(71), INQ(91),  \
			INQ(44), INQ(74), INQ(89), INQ(46), INQ(72), INQ(85),  \
			INQ(43), INQ(77), INQ(89), INQ(48), INQ(77), INQ(89),  \
			INQ(48), INQ(54), INQ(64), INQ(26), INQ(91), INQ(88),  \
			INQ(68), INQ(189), INQ(181), INQ(167), INQ(194),       \
			INQ(186), INQ(172), INQ(164), INQ(155), INQ(141),      \
			INQ(140), INQ(133), INQ(118), INQ(85), INQ(93),        \
			INQ(62), INQ(62), INQ(74), INQ(37), INQ(63), INQ(74),  \
			INQ(34), INQ(75), INQ(87), INQ(43), INQ(75), INQ(88),  \
			INQ(43), INQ(71), INQ(87), INQ(43), INQ(74), INQ(84),  \
			INQ(42), INQ(52), INQ(53), INQ(19), INQ(86), INQ(76),  \
			INQ(56), INQ(152), INQ(139), INQ(131), INQ(177),       \
			INQ(170), INQ(163), INQ(164), INQ(157), INQ(149),      \
			INQ(135), INQ(127), INQ(118), INQ(103), INQ(95),       \
			INQ(86), INQ(111), INQ(99), INQ(84), INQ(155),         \
			INQ(132), INQ(105), INQ(156), INQ(126), INQ(99),       \
			INQ(162), INQ(124), INQ(99), INQ(70), INQ(92),         \
			INQ(42), INQ(72), INQ(90), INQ(42), INQ(73), INQ(87),  \
			INQ(40), INQ(74), INQ(84), INQ(39), INQ(80), INQ(86),  \
			INQ(48), INQ(87), INQ(91), INQ(56), INQ(97), INQ(100), \
			INQ(65), INQ(104), INQ(108), INQ(73), INQ(86),         \
			INQ(88), INQ(56), INQ(103), INQ(96), INQ(78),          \
			INQ(190), INQ(181), INQ(167), INQ(187), INQ(178),      \
			INQ(164), INQ(161), INQ(152), INQ(137), INQ(162),      \
			INQ(154), INQ(139), INQ(106), INQ(102), INQ(80),       \
			INQ(79), INQ(76), INQ(50), INQ(79), INQ(76), INQ(48),  \
			INQ(89), INQ(88), INQ(55), INQ(89), INQ(87), INQ(53),  \
			INQ(82), INQ(78), INQ(44), INQ(82), INQ(77), INQ(42),  \
			INQ(73), INQ(65), INQ(36), INQ(56), INQ(42), INQ(28),  \
			INQ(66), INQ(53), INQ(55), INQ(71), INQ(69), INQ(84),  \
			INQ(47), INQ(46), INQ(64), INQ(41), INQ(40), INQ(57),  \
			INQ(44), INQ(43), INQ(61), INQ(45), INQ(42), INQ(53),  \
			INQ(80), INQ(69), INQ(61), INQ(125), INQ(105),         \
			INQ(91), INQ(159), INQ(127), INQ(107), INQ(70),        \
			INQ(91), INQ(45), INQ(74), INQ(87), INQ(44), INQ(84),  \
			INQ(88), INQ(48), INQ(95), INQ(95), INQ(57), INQ(106), \
			INQ(101), INQ(71), INQ(115), INQ(108), INQ(81),        \
			INQ(117), INQ(110), INQ(83), INQ(125), INQ(118),       \
			INQ(90), INQ(114), INQ(106), INQ(81), INQ(121),        \
			INQ(111), INQ(93), INQ(197), INQ(185), INQ(169),       \
			INQ(185), INQ(174), INQ(157), INQ(160), INQ(149),      \
			INQ(132), INQ(152), INQ(140), INQ(124), INQ(115),      \
			INQ(99), INQ(85), INQ(104), INQ(86), INQ(72), INQ(98), \
			INQ(81), INQ(66), INQ(99), INQ(81), INQ(66), INQ(96),  \
			INQ(77), INQ(61), INQ(96), INQ(72), INQ(54), INQ(93),  \
			INQ(72), INQ(52), INQ(85), INQ(70), INQ(53), INQ(80),  \
			INQ(68), INQ(63), INQ(49), INQ(43), INQ(52), INQ(24),  \
			INQ(32), INQ(55), INQ(23), INQ(33), INQ(57), INQ(26),  \
			INQ(36), INQ(60), INQ(26), INQ(36), INQ(61), INQ(26),  \
			INQ(34), INQ(57), INQ(31), INQ(32), INQ(48), INQ(59),  \
			INQ(50), INQ(56), INQ(111), INQ(92), INQ(82), INQ(75), \
			INQ(88), INQ(53), INQ(84), INQ(90), INQ(58), INQ(101), \
			INQ(100), INQ(70), INQ(111), INQ(105), INQ(78),        \
			INQ(109), INQ(98), INQ(76), INQ(109), INQ(97),         \
			INQ(77), INQ(111), INQ(98), INQ(79), INQ(114),         \
			INQ(102), INQ(82), INQ(106), INQ(93), INQ(74),         \
			INQ(118), INQ(106), INQ(87), INQ(190), INQ(177),       \
			INQ(159), INQ(187), INQ(174), INQ(156), INQ(151),      \
			INQ(139), INQ(121), INQ(126), INQ(113), INQ(95),       \
			INQ(110), INQ(92), INQ(78), INQ(111), INQ(90),         \
			INQ(78), INQ(99), INQ(78), INQ(68), INQ(98), INQ(77),  \
			INQ(69), INQ(103), INQ(80), INQ(74), INQ(104),         \
			INQ(76), INQ(70), INQ(102), INQ(80), INQ(71), INQ(90), \
			INQ(79), INQ(71), INQ(62), INQ(58), INQ(60), INQ(33),  \
			INQ(38), INQ(49), INQ(21), INQ(38), INQ(51), INQ(21),  \
			INQ(39), INQ(52), INQ(21), INQ(39), INQ(52), INQ(20),  \
			INQ(38), INQ(50), INQ(22), INQ(38), INQ(55), INQ(25),  \
			INQ(34), INQ(60), INQ(28), INQ(30), INQ(47), INQ(43),  \
			INQ(37), INQ(41), INQ(91), INQ(91), INQ(73), INQ(104), \
			INQ(104), INQ(86), INQ(108), INQ(105), INQ(88),        \
			INQ(104), INQ(99), INQ(83), INQ(100), INQ(92),         \
			INQ(75), INQ(100), INQ(90), INQ(73), INQ(100),         \
			INQ(90), INQ(73), INQ(103), INQ(93), INQ(76), INQ(95), \
			INQ(85), INQ(67), INQ(101), INQ(87), INQ(68),          \
			INQ(179), INQ(164), INQ(144), INQ(189), INQ(174),      \
			INQ(154), INQ(133), INQ(118), INQ(98), INQ(91),        \
			INQ(77), INQ(56), INQ(99), INQ(87), INQ(66), INQ(115), \
			INQ(102), INQ(85), INQ(110), INQ(97), INQ(83),         \
			INQ(113), INQ(99), INQ(90), INQ(123), INQ(108),        \
			INQ(103), INQ(116), INQ(102), INQ(99), INQ(108),       \
			INQ(101), INQ(94), INQ(76), INQ(78), INQ(73), INQ(34), \
			INQ(42), INQ(49), INQ(23), INQ(36), INQ(51), INQ(21),  \
			INQ(39), INQ(51), INQ(20), INQ(39), INQ(50), INQ(20),  \
			INQ(38), INQ(50), INQ(22), INQ(40), INQ(52), INQ(20),  \
			INQ(37), INQ(52), INQ(22), INQ(32), INQ(56), INQ(24),  \
			INQ(31), INQ(53), INQ(24), INQ(30), INQ(46), INQ(105), \
			INQ(98), INQ(88), INQ(103), INQ(98), INQ(88),          \
			INQ(102), INQ(99), INQ(88), INQ(96), INQ(93), INQ(82), \
			INQ(95), INQ(88), INQ(75), INQ(94), INQ(86), INQ(74),  \
			INQ(95), INQ(87), INQ(77), INQ(97), INQ(88), INQ(77),  \
			INQ(96), INQ(84), INQ(71), INQ(117), INQ(102),         \
			INQ(81), INQ(176), INQ(162), INQ(139), INQ(175),       \
			INQ(163), INQ(141), INQ(106), INQ(94), INQ(74),        \
			INQ(68), INQ(59), INQ(38), INQ(102), INQ(95), INQ(72), \
			INQ(116), INQ(109), INQ(88), INQ(114), INQ(106),       \
			INQ(88), INQ(125), INQ(117), INQ(101), INQ(135),       \
			INQ(126), INQ(113), INQ(126), INQ(120), INQ(111),      \
			INQ(97), INQ(98), INQ(95), INQ(39), INQ(48), INQ(53),  \
			INQ(19), INQ(33), INQ(45), INQ(19), INQ(35), INQ(52),  \
			INQ(18), INQ(33), INQ(54), INQ(18), INQ(34), INQ(55),  \
			INQ(20), INQ(35), INQ(56), INQ(23), INQ(39), INQ(59),  \
			INQ(23), INQ(37), INQ(59), INQ(22), INQ(34), INQ(54),  \
			INQ(17), INQ(33), INQ(49), INQ(22), INQ(33), INQ(55),  \
			INQ(106), INQ(97), INQ(88), INQ(103), INQ(96),         \
			INQ(86), INQ(103), INQ(99), INQ(88), INQ(104),         \
			INQ(100), INQ(88), INQ(109), INQ(98), INQ(88),         \
			INQ(108), INQ(98), INQ(92), INQ(102), INQ(94),         \
			INQ(92), INQ(105), INQ(93), INQ(89), INQ(119),         \
			INQ(102), INQ(92), INQ(151), INQ(133), INQ(111),       \
			INQ(179), INQ(164), INQ(140), INQ(147), INQ(136),      \
			INQ(115), INQ(105), INQ(97), INQ(79), INQ(109),        \
			INQ(103), INQ(86), INQ(125), INQ(112), INQ(96),        \
			INQ(129), INQ(115), INQ(99), INQ(130), INQ(117),       \
			INQ(100), INQ(129), INQ(117), INQ(99), INQ(124),       \
			INQ(113), INQ(93), INQ(122), INQ(112), INQ(96),        \
			INQ(75), INQ(75), INQ(77), INQ(20), INQ(29), INQ(49),  \
			INQ(17), INQ(32), INQ(51), INQ(19), INQ(34), INQ(49),  \
			INQ(18), INQ(32), INQ(54), INQ(19), INQ(34), INQ(57),  \
			INQ(20), INQ(35), INQ(58), INQ(18), INQ(34), INQ(57),  \
			INQ(21), INQ(33), INQ(58), INQ(20), INQ(34), INQ(52),  \
			INQ(14), INQ(37), INQ(43), INQ(21), INQ(32), INQ(55),  \
			INQ(114), INQ(103), INQ(91), INQ(114), INQ(104),       \
			INQ(92), INQ(120), INQ(110), INQ(98), INQ(125),        \
			INQ(114), INQ(102), INQ(132), INQ(117), INQ(102),      \
			INQ(125), INQ(113), INQ(102), INQ(112), INQ(105),      \
			INQ(97), INQ(115), INQ(104), INQ(94), INQ(139),        \
			INQ(121), INQ(106), INQ(164), INQ(140), INQ(120),      \
			INQ(163), INQ(139), INQ(119), INQ(129), INQ(108),      \
			INQ(90), INQ(129), INQ(110), INQ(95), INQ(144),        \
			INQ(125), INQ(112), INQ(142), INQ(117), INQ(106),      \
			INQ(139), INQ(114), INQ(102), INQ(134), INQ(113),      \
			INQ(99), INQ(125), INQ(107), INQ(93), INQ(125),        \
			INQ(109), INQ(94), INQ(112), INQ(103), INQ(92),        \
			INQ(50), INQ(52), INQ(57), INQ(20), INQ(30), INQ(51),  \
			INQ(20), INQ(35), INQ(56), INQ(22), INQ(36), INQ(53),  \
			INQ(21), INQ(36), INQ(58), INQ(22), INQ(37), INQ(60),  \
			INQ(24), INQ(38), INQ(61), INQ(21), INQ(36), INQ(59),  \
			INQ(21), INQ(34), INQ(57), INQ(36), INQ(47), INQ(61),  \
			INQ(37), INQ(50), INQ(55), INQ(22), INQ(30), INQ(51),  \
			INQ(136), INQ(121), INQ(106), INQ(131), INQ(115),      \
			INQ(100), INQ(133), INQ(114), INQ(100), INQ(134),      \
			INQ(113), INQ(99), INQ(130), INQ(108), INQ(91),        \
			INQ(129), INQ(113), INQ(97), INQ(126), INQ(117),       \
			INQ(104), INQ(118), INQ(109), INQ(92), INQ(126),       \
			INQ(109), INQ(89), INQ(145), INQ(115), INQ(100),       \
			INQ(142), INQ(108), INQ(94), INQ(141), INQ(108),       \
			INQ(95), INQ(150), INQ(117), INQ(104), INQ(147),       \
			INQ(113), INQ(101), INQ(143), INQ(108), INQ(98),       \
			INQ(144), INQ(111), INQ(102), INQ(140), INQ(112),      \
			INQ(104), INQ(141), INQ(118), INQ(110), INQ(137),      \
			INQ(119), INQ(112), INQ(74), INQ(69), INQ(67),         \
			INQ(30), INQ(36), INQ(45), INQ(23), INQ(37), INQ(56),  \
			INQ(23), INQ(37), INQ(60), INQ(26), INQ(38), INQ(60),  \
			INQ(24), INQ(39), INQ(61), INQ(25), INQ(40), INQ(63),  \
			INQ(25), INQ(40), INQ(62), INQ(23), INQ(38), INQ(61),  \
			INQ(24), INQ(39), INQ(59), INQ(80), INQ(84), INQ(92),  \
			INQ(78), INQ(72), INQ(76), INQ(28), INQ(31), INQ(50)   \
	}

/* Output classes Probability vector: reference for img #12 */

#define OUT_PROB_12_SHAPE                                                      \
	{                                                                      \
		10                                                             \
	}
#define OUT_PROB_12_RANK (1)
#define OUT_PROB_12                                                            \
	{                                                                      \
		PBQ(0.000184378), PBQ(0.000350850), PBQ(0.028723521),          \
			PBQ(0.274058372), PBQ(0.119976416), PBQ(0.439902395),  \
			PBQ(0.122212067), PBQ(0.014322803), PBQ(0.000122589),  \
			PBQ(0.000146562)                                       \
	}

#endif /* _CIFAR10_REF_INOUT_H_ */
